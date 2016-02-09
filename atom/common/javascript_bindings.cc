// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>
#include <string>
#include <vector>

#include "atom/common/api/api_messages.h"
#include "atom/common/asar/asar_util.h"
#include "atom/common/javascript_bindings.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "atom/renderer/api/atom_api_web_frame.h"
#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/files/file_path.h"
#include "content/public/renderer/render_view.h"
#include "native_mate/dictionary.h"
#include "net/base/filename_util.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace {
  // TODO(bridiver) This is copied from atom_api_renderer_ipc.cc
  // and atom_api_v8_util.cc and should be cleaned up
  // using
  // v8::Local<v8::Value> unused = v8::Undefined(isolate);
  // v8::Local<v8::Object> v8_util = v8::Object::New(isolate);
  // auto mod = node::get_builtin_module("atom_common_v8_util");
  // mod->nm_context_register_func(v8_util, unused, context, nullptr);
  // binding.Set("v8_util", v8_util);
  // results in an unresolved external symbol error for get_builtin_module
  // on windows
  content::RenderView* GetCurrentRenderView() {
    blink::WebLocalFrame* frame =
      blink::WebLocalFrame::frameForCurrentContext();
    if (!frame)
      return NULL;

    blink::WebView* view = frame->view();
    if (!view)
      return NULL;  // can happen during closing.

    return content::RenderView::FromWebView(view);
  }

  void Send(mate::Arguments* args,
            const base::string16& channel,
            const base::ListValue& arguments) {
    content::RenderView* render_view = GetCurrentRenderView();
    if (render_view == NULL)
      return;

    bool success = render_view->Send(new AtomViewHostMsg_Message(
        render_view->GetRoutingID(), channel, arguments));

    if (!success)
      args->ThrowError("Unable to send AtomViewHostMsg_Message");
  }

  base::string16 SendSync(mate::Arguments* args,
                          const base::string16& channel,
                          const base::ListValue& arguments) {
    base::string16 json;

    content::RenderView* render_view = GetCurrentRenderView();
    if (render_view == NULL)
      return json;

    IPC::SyncMessage* message = new AtomViewHostMsg_Message_Sync(
        render_view->GetRoutingID(), channel, arguments, &json);
    bool success = render_view->Send(message);

    if (!success)
      args->ThrowError("Unable to send AtomViewHostMsg_Message_Sync");

    return json;
  }

  v8::Local<v8::Value> GetHiddenValue(v8::Local<v8::Object> object,
                                    v8::Local<v8::String> key) {
    return object->GetHiddenValue(key);
  }

  void SetHiddenValue(v8::Local<v8::Object> object,
                      v8::Local<v8::String> key,
                      v8::Local<v8::Value> value) {
    object->SetHiddenValue(key, value);
  }

  // TODO(bridiver) This is mostly copied from node_bindings.cc
  // and should be cleaned up
  base::FilePath GetResourcesPath() {
    auto command_line = base::CommandLine::ForCurrentProcess();
    base::FilePath exec_path(command_line->GetProgram());
    PathService::Get(base::FILE_EXE, &exec_path);
    base::FilePath resources_path =
  #if defined(OS_MACOSX)
        exec_path.DirName().DirName().DirName().DirName().DirName()
                .Append("Resources");
  #else
        exec_path.DirName().Append(FILE_PATH_LITERAL("resources"));
  #endif
    return resources_path;
  }

  std::string ExceptionToString(const v8::TryCatch& try_catch) {
    std::string str;
    v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
    v8::String::Utf8Value exception(try_catch.Exception());
    v8::Local<v8::Message> message(try_catch.Message());
    if (message.IsEmpty()) {
      str.append(base::StringPrintf("%s\n", *exception));
    } else {
      v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
      int linenum = message->GetLineNumber();
      int colnum = message->GetStartColumn();
      str.append(base::StringPrintf(
          "%s:%i:%i %s\n", *filename, linenum, colnum, *exception));
      v8::String::Utf8Value sourceline(message->GetSourceLine());
      str.append(base::StringPrintf("%s\n", *sourceline));
    }
    return str;
  }

  v8::Handle<v8::Value> ExecuteScriptFile(v8::Isolate* isolate,
                                              base::FilePath script_path,
                                              std::string script_source) {
    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(isolate,
                                script_source.data(),
                                v8::String::kNormalString,
                                script_source.size());

    std::string script_name = script_path.AsUTF8Unsafe();
    v8::Local<v8::String> name =
        v8::String::NewFromUtf8(isolate,
                                script_name.data(),
                                v8::String::kNormalString,
                                script_name.size());

    v8::TryCatch try_catch;
    v8::Local<v8::Script> script = v8::Script::Compile(source, name);
    if (script.IsEmpty() || try_catch.HasCaught()) {
      LOG(FATAL) << "Failed to parse script file " << script_name;
      LOG(FATAL) << ExceptionToString(try_catch);
      exit(3);
    }

    v8::Local<v8::Value> result = script->Run();
    if (result.IsEmpty() || try_catch.HasCaught()) {
      LOG(FATAL) << "Failed to execute script file " << script_name;
      LOG(FATAL) << ExceptionToString(try_catch);
      exit(4);
    }

    return result;
  }

  base::LazyInstance<std::vector<std::pair<base::FilePath, std::string>>>
    scripts = LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace atom {

JavascriptBindings::JavascriptBindings() {
}

JavascriptBindings::~JavascriptBindings() {
}

void JavascriptBindings::PreSandboxStartup() {
  // Load everything
  base::FilePath resources_path = GetResourcesPath();
  base::FilePath script_path =
          resources_path.Append(FILE_PATH_LITERAL("atom.asar"))
                        .Append(FILE_PATH_LITERAL("renderer"))
                        .Append(FILE_PATH_LITERAL("lib"))
                        .Append(FILE_PATH_LITERAL("init-without-node.js"));

  std::string script_source;
  asar::ReadFileToString(script_path, &script_source);
  scripts.Get().push_back(std::make_pair(script_path, script_source));

  // Load supplied contentScripts
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(switches::kContentScripts)) {
    std::stringstream ss(cmd_line->
                              GetSwitchValueASCII(switches::kContentScripts));

    std::string name;
    while (std::getline(ss, name, ',')) {
      base::FilePath script_path;
      net::FileURLToFilePath(GURL(name), &script_path);

      asar::ReadFileToString(script_path, &script_source);
      scripts.Get().push_back(std::make_pair(script_path, script_source));
    }
  }
}

// bind native functions
void JavascriptBindings::BindTo(v8::Isolate* isolate,
                          v8::Local<v8::Object> process) {
  mate::Dictionary binding(isolate, v8::Object::New(isolate));

  mate::Dictionary v8_util(isolate, v8::Object::New(isolate));
  v8_util.SetMethod("setHiddenValue", &SetHiddenValue);
  v8_util.SetMethod("getHiddenValue", &GetHiddenValue);
  binding.Set("v8_util", v8_util.GetHandle());

  mate::Dictionary ipc(isolate, v8::Object::New(isolate));
  ipc.SetMethod("send", &Send);
  ipc.SetMethod("sendSync", &SendSync);
  binding.Set("ipc", ipc.GetHandle());

  mate::Dictionary web_frame(isolate, v8::Object::New(isolate));
  web_frame.Set("webFrame", api::WebFrame::Create(isolate));
  binding.Set("web_frame", web_frame);

  mate::Dictionary dict(isolate, process);
  dict.Set("binding", binding);

  // store in the global scope
  v8::Local<v8::String> process_key = mate::StringToV8(isolate, "process");
  process->CreationContext()->Global()->Set(process_key, process);

  for (std::vector<std::pair<base::FilePath, std::string>>::iterator i =
             scripts.Get().begin();
         i != scripts.Get().end();
         ++i) {
    ExecuteScriptFile(isolate, i->first, i->second);
  }

  // remove process object from the global scope
  process->CreationContext()->Global()->Delete(process_key);
}

}  // namespace atom
