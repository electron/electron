// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_renderer_client.h"

#include <string>
#include <vector>

#include "atom/common/api/api_messages.h"
#include "atom/common/api/atom_bindings.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/asar/asar_util.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_bindings.h"
#include "atom/common/node_includes.h"
#include "atom/common/options_switches.h"
#include "atom/renderer/api/atom_api_web_frame.h"
#include "atom/renderer/atom_render_view_observer.h"
#include "atom/renderer/guest_view_container.h"
#include "atom/renderer/node_array_buffer_bridge.h"
#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "chrome/renderer/media/chrome_key_systems.h"
#include "chrome/renderer/pepper/pepper_helper.h"
#include "chrome/renderer/printing/print_web_view_helper.h"
#include "chrome/renderer/tts_dispatcher.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebCustomElement.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginParams.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"
#include "third_party/WebKit/public/web/WebRuntimeFeatures.h"
#include "third_party/WebKit/public/web/WebView.h"

#if defined(OS_WIN)
#include <shlobj.h>
#endif

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

  // TODO(bridiver) create a separate file for script functions
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

  v8::Handle<v8::Value> ExecuteScriptFile(v8::Handle<v8::Context> context,
                                              base::FilePath script_path) {
    v8::Isolate* isolate = context->GetIsolate();
    std::string script_source;
    asar::ReadFileToString(script_path, &script_source);

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
    if (script.IsEmpty()) {
      LOG(FATAL) << "Failed to parse script file " << script_name;
      LOG(FATAL) << ExceptionToString(try_catch);
      exit(3);
    }

    v8::Local<v8::Value> result = script->Run();
    if (result.IsEmpty()) {
      LOG(FATAL) << "Failed to execute script file " << script_name;
      LOG(FATAL) << ExceptionToString(try_catch);
      exit(4);
    }

    return result;
  }
}  // namespace

namespace atom {

namespace {

// Helper class to forward the messages to the client.
class AtomRenderFrameObserver : public content::RenderFrameObserver {
 public:
  AtomRenderFrameObserver(content::RenderFrame* frame,
                          AtomRendererClient* renderer_client)
      : content::RenderFrameObserver(frame),
        world_id_(-1),
        renderer_client_(renderer_client) {}

  // content::RenderFrameObserver:
  void DidCreateScriptContext(v8::Handle<v8::Context> context,
                              int extension_group,
                              int world_id) override {
    if (world_id_ != -1 && world_id_ != world_id)
      return;
    world_id_ = world_id;
    renderer_client_->DidCreateScriptContext(
		render_frame()->GetWebFrame(), context);
  }
  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                int world_id) override {
    if (world_id_ != world_id)
      return;
    renderer_client_->WillReleaseScriptContext(
		render_frame()->GetWebFrame(), context);
  }

 private:
  int world_id_;
  AtomRendererClient* renderer_client_;

  DISALLOW_COPY_AND_ASSIGN(AtomRenderFrameObserver);
};

}  // namespace

AtomRendererClient::AtomRendererClient()
    : node_bindings_(NodeBindings::Create(false)),
      atom_bindings_(new AtomBindings),
      run_node_(true) {
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  // if node integration is disabled and there is
  // no preload script then don't start node
  if ((!cmd_line->HasSwitch(switches::kNodeIntegration) ||
       cmd_line->GetSwitchValueASCII(switches::kNodeIntegration) == "false") &&
      !cmd_line->HasSwitch(switches::kPreloadScript) &&
      !cmd_line->HasSwitch(switches::kPreloadURL)) {
    run_node_ = false;
  }
}

AtomRendererClient::~AtomRendererClient() {
}

void AtomRendererClient::WebKitInitialized() {
  blink::WebCustomElement::addEmbedderCustomElementName("webview");
  blink::WebCustomElement::addEmbedderCustomElementName("browserplugin");

  if (!run_node_)
    return;

  OverrideNodeArrayBuffer();

  node_bindings_->Initialize();
  node_bindings_->PrepareMessageLoop();

  DCHECK(!global_env);

  // Create a default empty environment which would be used when we need to
  // run V8 code out of a window context (like running a uv callback).
  v8::Isolate* isolate = blink::mainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate);
  global_env = node::Environment::New(context, uv_default_loop());
}

void AtomRendererClient::RenderThreadStarted() {
  content::RenderThread::Get()->AddObserver(this);

#if defined(OS_WIN)
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  base::string16 app_id =
      command_line->GetSwitchValueNative(switches::kAppUserModelId);
  if (!app_id.empty()) {
    SetCurrentProcessExplicitAppUserModelID(app_id.c_str());
  }
#endif
}

void AtomRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new PepperHelper(render_frame);

  // Allow file scheme to handle service worker by default.
  blink::WebSecurityPolicy::registerURLSchemeAsAllowingServiceWorkers("file");

  // Only insert node integration for the main frame.
  if (!render_frame->IsMainFrame())
    return;

  new AtomRenderFrameObserver(render_frame, this);
}

void AtomRendererClient::RenderViewCreated(content::RenderView* render_view) {
  new printing::PrintWebViewHelper(render_view);
  new AtomRenderViewObserver(render_view, this);
}

blink::WebSpeechSynthesizer* AtomRendererClient::OverrideSpeechSynthesizer(
    blink::WebSpeechSynthesizerClient* client) {
  return new TtsDispatcher(client);
}

bool AtomRendererClient::OverrideCreatePlugin(
    content::RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params,
    blink::WebPlugin** plugin) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (params.mimeType.utf8() == content::kBrowserPluginMimeType ||
      command_line->HasSwitch(switches::kEnablePlugins))
    return false;

  *plugin = nullptr;
  return true;
}

void AtomRendererClient::DidCreateScriptContext(
    blink::WebFrame* frame,
    v8::Handle<v8::Context> context) {
  GURL url(frame->document().url());

  if (url == GURL(content::kSwappedOutURL))
      return;

  if (run_node_) {
    // only load node in the main frame
    if (frame->parent())
      return;

    // Give the node loop a run to make sure everything is ready.
    node_bindings_->RunMessageLoop();

    // Setup node environment for each window.
    node::Environment* env = node_bindings_->CreateEnvironment(context);

    // Add atom-shell extended APIs.
    atom_bindings_->BindTo(env->isolate(), env->process_object());

    // Make uv loop being wrapped by window context.
    if (node_bindings_->uv_env() == nullptr)
      node_bindings_->set_uv_env(env);

    // Load everything.
    node_bindings_->LoadEnvironment(env);
  } else {
    v8::Isolate* isolate = context->GetIsolate();
    v8::HandleScope handle_scope(isolate);

    // Create a process object
    mate::Dictionary process(isolate, v8::Object::New(isolate));

    // bind native functions
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

    process.Set("binding", binding);

    // attach the atom bindings
    atom_bindings_->BindTo(isolate, process.GetHandle());

    // store in the global scope
    v8::Local<v8::String> process_key = mate::StringToV8(isolate, "process");
    context->Global()->Set(process_key, process.GetHandle());

    // Load everything
    base::FilePath resources_path = GetResourcesPath();
    base::FilePath script_path =
            resources_path.Append(FILE_PATH_LITERAL("atom.asar"))
                          .Append(FILE_PATH_LITERAL("renderer"))
                          .Append(FILE_PATH_LITERAL("lib"))
                          .Append(FILE_PATH_LITERAL("init-without-node.js"));
    ExecuteScriptFile(context, script_path);

    // Run supplied contentScripts
    base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
    if (cmd_line->HasSwitch(switches::kContentScripts)) {
      std::stringstream ss(cmd_line->
                              GetSwitchValueASCII(switches::kContentScripts));
      std::string name;
      while (std::getline(ss, name, ',')) {
        ExecuteScriptFile(context, base::FilePath::FromUTF8Unsafe(name));
      }
    }
    // remove process object from the global scope
    context->Global()->Delete(process_key);
  }
}

void AtomRendererClient::WillReleaseScriptContext(
    v8::Handle<v8::Context> context) {
  node::Environment* env = node::Environment::GetCurrent(context);
  if (env != nullptr && env == node_bindings_->uv_env()) {
    node_bindings_->set_uv_env(nullptr);
    mate::EmitEvent(env->isolate(), env->process_object(), "exit");
  }
}

bool AtomRendererClient::ShouldFork(blink::WebLocalFrame* frame,
                                    const GURL& url,
                                    const std::string& http_method,
                                    bool is_initial_navigation,
                                    bool is_server_redirect,
                                    bool* send_referrer) {
  if (run_node_) {
    *send_referrer = true;
    return http_method == "GET" && !is_server_redirect;
  }

  return false;
}

content::BrowserPluginDelegate* AtomRendererClient::CreateBrowserPluginDelegate(
    content::RenderFrame* render_frame,
    const std::string& mime_type,
    const GURL& original_url) {
  if (mime_type == content::kBrowserPluginMimeType) {
    return new GuestViewContainer(render_frame);
  } else {
    return nullptr;
  }
}

void AtomRendererClient::AddKeySystems(
    std::vector<media::KeySystemInfo>* key_systems) {
  AddChromeKeySystems(key_systems);
}

}  // namespace atom
