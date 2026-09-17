// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/preload_environment.h"

#include <array>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/strings/strcat.h"
#include "gin/converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/node_event_emitter.h"
#include "shell/renderer/preload_utils.h"
#include "v8/include/v8-container.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-object.h"
#include "v8/include/v8-primitive.h"

namespace electron::preload_environment {

namespace {

constexpr std::string_view kProcessKey = "electron:preload-process";

// --- the `electron` module --------------------------------------------------

struct ModuleEntry {
  const char* name;
  const char* binding;
  // Property of the binding's exports that is the module, or null when the
  // exports object itself is.
  const char* key;
  bool renderer_only;
};

constexpr auto kModules = std::to_array<ModuleEntry>({
    {"contextBridge", "electron_renderer_context_bridge", "contextBridge",
     false},
    {"crashReporter", "electron_renderer_crash_reporter", nullptr, true},
    {"ipcRenderer", "electron_renderer_ipc", "ipcRenderer", false},
    {"nativeImage", "electron_common_native_image", "nativeImage", false},
    {"sharedTexture", "electron_common_shared_texture", nullptr, true},
    {"webFrame", "electron_renderer_web_frame", "mainFrame", true},
    {"webUtils", "electron_renderer_web_utils", nullptr, true},
});

v8::Local<v8::Value> LoadBinding(v8::Isolate* isolate, const char* name) {
  return preload_utils::GetBinding(isolate, gin::StringToV8(isolate, name));
}

void ModuleGetter(v8::Local<v8::Name> property,
                  const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  const ModuleEntry& entry =
      kModules[static_cast<size_t>(info.Data().As<v8::Int32>()->Value())];
  v8::Local<v8::Value> exports = LoadBinding(isolate, entry.binding);
  if (exports.IsEmpty() || !exports->IsObject())
    return;
  v8::Local<v8::Value> value = exports;
  if (std::string_view(entry.name) == "sharedTexture") {
    // { subtle, setSharedTextureReceiver }; the transfer itself is handled by
    // ElectronApiServiceImpl::ReceiveSharedTexture().
    v8::Local<v8::Value> set_receiver;
    if (!exports.As<v8::Object>()
             ->Get(context,
                   gin::StringToSymbol(isolate, "setSharedTextureReceiver"))
             .ToLocal(&set_receiver)) {
      return;
    }
    auto shared_texture = gin_helper::Dictionary::CreateEmpty(isolate);
    shared_texture.Set("subtle", exports);
    shared_texture.Set("setSharedTextureReceiver", set_receiver);
    value = shared_texture.GetHandle();
  } else if (entry.key &&
             !exports.As<v8::Object>()
                  ->Get(context, gin::StringToSymbol(isolate, entry.key))
                  .ToLocal(&value)) {
    return;
  }
  info.GetReturnValue().Set(value);
}

// --- require
// ------------------------------------------------------------------

void Require(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Array> data = info.Data().As<v8::Array>();
  v8::Local<v8::Value> module, renderer_flavor;
  if (!data->Get(context, 0).ToLocal(&module) ||
      !data->Get(context, 1).ToLocal(&renderer_flavor)) {
    return;
  }
  std::string name;
  if (info.Length() > 0) {
    v8::Local<v8::String> name_string;
    if (info[0]->ToString(context).ToLocal(&name_string))
      name = gin::V8ToString(isolate, name_string);
  }
  if (name == "electron" || name == "electron/common" ||
      (renderer_flavor->IsTrue() && name == "electron/renderer")) {
    info.GetReturnValue().Set(module);
    return;
  }
  isolate->ThrowException(v8::Exception::Error(
      gin::StringToV8(isolate, base::StrCat({"module not found: ", name}))));
}

// --- process
// ------------------------------------------------------------------

// ipcRendererInternal[method](...args) in the current context.
v8::MaybeLocal<v8::Value> CallIpcRendererInternal(
    v8::Isolate* isolate,
    const char* method,
    base::span<v8::Local<v8::Value>> args) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> exports = LoadBinding(isolate, "electron_renderer_ipc");
  v8::Local<v8::Value> ipc, fn;
  if (exports.IsEmpty() || !exports->IsObject() ||
      !exports.As<v8::Object>()
           ->Get(context, gin::StringToSymbol(isolate, "ipcRendererInternal"))
           .ToLocal(&ipc) ||
      !ipc->IsObject() ||
      !ipc.As<v8::Object>()
           ->Get(context, gin::StringToSymbol(isolate, method))
           .ToLocal(&fn) ||
      !fn->IsFunction()) {
    return {};
  }
  return fn.As<v8::Function>()->Call(
      context, ipc, static_cast<int>(args.size()), args.data());
}

void GetProcessMemoryInfo(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  std::array<v8::Local<v8::Value>, 1> args{
      gin::StringToV8(isolate, "BROWSER_GET_PROCESS_MEMORY_INFO")};
  v8::Local<v8::Value> result;
  if (CallIpcRendererInternal(isolate, "invoke", args).ToLocal(&result))
    info.GetReturnValue().Set(result);
}

// --- running scripts ---------------------------------------------------------

// console.error(...args)
void ConsoleError(v8::Local<v8::Context> context,
                  base::span<v8::Local<v8::Value>> args) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Value> console, error;
  if (!context->Global()
           ->Get(context, gin::StringToSymbol(isolate, "console"))
           .ToLocal(&console) ||
      !console->IsObject() ||
      !console.As<v8::Object>()
           ->Get(context, gin::StringToSymbol(isolate, "error"))
           .ToLocal(&error) ||
      !error->IsFunction()) {
    return;
  }
  std::ignore = error.As<v8::Function>()->Call(
      context, console, static_cast<int>(args.size()), args.data());
}

void ReportPreloadError(v8::Local<v8::Context> context,
                        const std::string& file_path,
                        v8::Local<v8::Value> error) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::TryCatch try_catch(isolate);
  std::array<v8::Local<v8::Value>, 1> message{gin::StringToV8(
      isolate, base::StrCat({"Unable to load preload script: ", file_path}))};
  ConsoleError(context, message);
  std::array<v8::Local<v8::Value>, 1> error_arg{error};
  ConsoleError(context, error_arg);
  std::array<v8::Local<v8::Value>, 3> ipc_args{
      gin::StringToV8(isolate, "BROWSER_PRELOAD_ERROR"),
      gin::StringToV8(isolate, file_path), error};
  std::ignore = CallIpcRendererInternal(isolate, "send", ipc_args);
}

}  // namespace

v8::Local<v8::Object> CreateElectronModule(v8::Local<v8::Context> context,
                                           Flavor flavor) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Object> module = v8::Object::New(isolate);
  for (size_t i = 0; i < kModules.size(); ++i) {
    if (kModules[i].renderer_only && flavor != Flavor::kRenderer)
      continue;
    // Read-only like the getter-only accessors these used to be.
    module
        ->SetLazyDataProperty(
            context, gin::StringToSymbol(isolate, kModules[i].name),
            ModuleGetter, v8::Integer::New(isolate, static_cast<int>(i)),
            static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete))
        .Check();
  }
  return module;
}

v8::Local<v8::Object> CreateProcessObject(
    v8::Local<v8::Context> context,
    gin_helper::Dictionary base,
    const mojom::RendererStartupDataPtr& data) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Object> process = gin_helper::NewNodeEventEmitter(context);
  gin_helper::Dictionary dict(isolate, process);

  // Object.assign(process, base, data.process)
  v8::Local<v8::Object> base_object = base.GetHandle();
  v8::Local<v8::Array> keys;
  if (base_object->GetOwnPropertyNames(context).ToLocal(&keys)) {
    for (uint32_t i = 0; i < keys->Length(); ++i) {
      v8::Local<v8::Value> key, value;
      if (keys->Get(context, i).ToLocal(&key) &&
          base_object->Get(context, key).ToLocal(&value)) {
        process->CreateDataProperty(context, key.As<v8::Name>(), value).Check();
      }
    }
  }
  preload_utils::SetProcessProperties(isolate, &dict, data);

  dict.Set("getProcessMemoryInfo",
           v8::Function::New(context, GetProcessMemoryInfo, {}, 0,
                             v8::ConstructorBehavior::kThrow)
               .ToLocalChecked());
  // Like Node.js's --expose-internals: lets the tests reach internal bindings.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          "unsafely-expose-electron-internals-for-testing")) {
    dict.SetMethod("_linkedBinding", &preload_utils::GetBinding);
  }

  gin_helper::Dictionary(isolate, context->Global())
      .SetHidden(kProcessKey, process.As<v8::Value>());
  return process;
}

void RunPreloadScripts(v8::Local<v8::Context> context,
                       content::RenderFrame* render_frame,
                       ServiceWorkerData* service_worker_data,
                       v8::Local<v8::Object> process,
                       v8::Local<v8::Object> electron_module,
                       Flavor flavor,
                       const mojom::RendererStartupDataPtr& data) {
  if (!data)
    return;
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  v8::Local<v8::Value> require_data_items[] = {
      electron_module, v8::Boolean::New(isolate, flavor == Flavor::kRenderer)};
  v8::Local<v8::Function> require =
      v8::Function::New(context, Require,
                        v8::Array::New(isolate, require_data_items, 2), 1,
                        v8::ConstructorBehavior::kThrow)
          .ToLocalChecked();
  require->SetName(gin::StringToSymbol(isolate, "require"));

  const std::vector<std::string> parameters = {"require", "process", "exports",
                                               "module", "global"};

  for (const auto& script : data->preload_scripts) {
    v8::TryCatch try_catch(isolate);
    if (script->error) {
      ReportPreloadError(
          context, script->file_path,
          v8::Exception::Error(gin::StringToV8(isolate, *script->error)));
      continue;
    }
    if (base::span<const uint8_t>(script->contents).empty())
      continue;
    v8::Local<v8::Value> fn = preload_utils::CreatePreloadScript(
        render_frame, service_worker_data, isolate, script->id, parameters);
    if (fn.IsEmpty() || !fn->IsFunction()) {
      if (try_catch.HasCaught())
        ReportPreloadError(context, script->file_path, try_catch.Exception());
      continue;
    }
    v8::Local<v8::Object> exports = v8::Object::New(isolate);
    auto module = gin_helper::Dictionary::CreateEmpty(isolate);
    module.Set("exports", exports);
    v8::Local<v8::Value> args[] = {require, process, exports,
                                   module.GetHandle(), context->Global()};
    if (fn.As<v8::Function>()
            ->Call(context, v8::Undefined(isolate), std::size(args), args)
            .IsEmpty() &&
        try_catch.HasCaught() && !try_catch.HasTerminated()) {
      ReportPreloadError(context, script->file_path, try_catch.Exception());
    }
  }
}

void EmitProcessEvent(v8::Local<v8::Context> context, std::string_view event) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  gin_helper::Dictionary global(isolate, context->Global());
  v8::Local<v8::Value> process;
  if (!global.GetHidden(kProcessKey, &process) || !process->IsObject())
    return;
  gin_helper::EmitEvent(isolate, process.As<v8::Object>(),
                        gin::StringToV8(isolate, event), {});
}

}  // namespace electron::preload_environment
