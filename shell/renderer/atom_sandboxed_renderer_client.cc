// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/atom_sandboxed_renderer_client.h"

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/process/process_handle.h"
#include "content/public/renderer/render_frame.h"
#include "electron/buildflags/buildflags.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/application_info.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/options_switches.h"
#include "shell/renderer/atom_render_frame_observer.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/electron_node/src/node_binding.h"

namespace electron {

namespace {

const char kLifecycleKey[] = "lifecycle";
const char kModuleCacheKey[] = "native-module-cache";

bool IsDevTools(content::RenderFrame* render_frame) {
  return render_frame->GetWebFrame()->GetDocument().Url().ProtocolIs(
      "devtools");
}

bool IsDevToolsExtension(content::RenderFrame* render_frame) {
  return render_frame->GetWebFrame()->GetDocument().Url().ProtocolIs(
      "chrome-extension");
}

v8::Local<v8::Object> GetModuleCache(v8::Isolate* isolate) {
  auto context = isolate->GetCurrentContext();
  gin_helper::Dictionary global(isolate, context->Global());
  v8::Local<v8::Value> cache;

  if (!global.GetHidden(kModuleCacheKey, &cache)) {
    cache = v8::Object::New(isolate);
    global.SetHidden(kModuleCacheKey, cache);
  }

  return cache->ToObject(context).ToLocalChecked();
}

// adapted from node.cc
v8::Local<v8::Value> GetBinding(v8::Isolate* isolate,
                                v8::Local<v8::String> key,
                                gin_helper::Arguments* margs) {
  v8::Local<v8::Object> exports;
  std::string module_key = gin::V8ToString(isolate, key);
  gin_helper::Dictionary cache(isolate, GetModuleCache(isolate));

  if (cache.Get(module_key.c_str(), &exports)) {
    return exports;
  }

  auto* mod = node::binding::get_linked_module(module_key.c_str());

  if (!mod) {
    char errmsg[1024];
    snprintf(errmsg, sizeof(errmsg), "No such module: %s", module_key.c_str());
    margs->ThrowError(errmsg);
    return exports;
  }

  exports = v8::Object::New(isolate);
  DCHECK_EQ(mod->nm_register_func, nullptr);
  DCHECK_NE(mod->nm_context_register_func, nullptr);
  mod->nm_context_register_func(exports, v8::Null(isolate),
                                isolate->GetCurrentContext(), mod->nm_priv);
  cache.Set(module_key.c_str(), exports);
  return exports;
}

v8::Local<v8::Value> CreatePreloadScript(v8::Isolate* isolate,
                                         v8::Local<v8::String> preloadSrc) {
  return RendererClientBase::RunScript(isolate->GetCurrentContext(),
                                       preloadSrc);
}

void InvokeHiddenCallback(v8::Handle<v8::Context> context,
                          const std::string& hidden_key,
                          const std::string& callback_name) {
  auto* isolate = context->GetIsolate();
  auto binding_key =
      gin::ConvertToV8(isolate, hidden_key)->ToString(context).ToLocalChecked();
  auto private_binding_key = v8::Private::ForApi(isolate, binding_key);
  auto global_object = context->Global();
  v8::Local<v8::Value> value;
  if (!global_object->GetPrivate(context, private_binding_key).ToLocal(&value))
    return;
  if (value.IsEmpty() || !value->IsObject())
    return;
  auto binding = value->ToObject(context).ToLocalChecked();
  auto callback_key = gin::ConvertToV8(isolate, callback_name)
                          ->ToString(context)
                          .ToLocalChecked();
  auto callback_value = binding->Get(context, callback_key).ToLocalChecked();
  DCHECK(callback_value->IsFunction());  // set by sandboxed_renderer/init.js
  auto callback = v8::Handle<v8::Function>::Cast(callback_value);
  ignore_result(callback->Call(context, binding, 0, nullptr));
}

}  // namespace

AtomSandboxedRendererClient::AtomSandboxedRendererClient() {
  // Explicitly register electron's builtin modules.
  NodeBindings::RegisterBuiltinModules();
  metrics_ = base::ProcessMetrics::CreateCurrentProcessMetrics();
}

AtomSandboxedRendererClient::~AtomSandboxedRendererClient() = default;

void AtomSandboxedRendererClient::InitializeBindings(
    v8::Local<v8::Object> binding,
    v8::Local<v8::Context> context,
    bool is_main_frame) {
  auto* isolate = context->GetIsolate();
  gin_helper::Dictionary b(isolate, binding);
  b.SetMethod("get", GetBinding);
  b.SetMethod("createPreloadScript", CreatePreloadScript);

  gin_helper::Dictionary process = gin::Dictionary::CreateEmpty(isolate);
  b.Set("process", process);

  ElectronBindings::BindProcess(isolate, &process, metrics_.get());

  process.Set("argv", base::CommandLine::ForCurrentProcess()->argv());
  process.SetReadOnly("pid", base::GetCurrentProcId());
  process.SetReadOnly("sandboxed", true);
  process.SetReadOnly("type", "renderer");
  process.SetReadOnly("isMainFrame", is_main_frame);
}

void AtomSandboxedRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new AtomRenderFrameObserver(render_frame, this);
  RendererClientBase::RenderFrameCreated(render_frame);
}

void AtomSandboxedRendererClient::RenderViewCreated(
    content::RenderView* render_view) {
  RendererClientBase::RenderViewCreated(render_view);
}

void AtomSandboxedRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  RendererClientBase::RunScriptsAtDocumentStart(render_frame);
  if (injected_frames_.find(render_frame) == injected_frames_.end())
    return;

  auto* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context =
      GetContext(render_frame->GetWebFrame(), isolate);
  v8::Context::Scope context_scope(context);

  InvokeHiddenCallback(context, kLifecycleKey, "onDocumentStart");
}

void AtomSandboxedRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  RendererClientBase::RunScriptsAtDocumentEnd(render_frame);
  if (injected_frames_.find(render_frame) == injected_frames_.end())
    return;

  auto* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context =
      GetContext(render_frame->GetWebFrame(), isolate);
  v8::Context::Scope context_scope(context);

  InvokeHiddenCallback(context, kLifecycleKey, "onDocumentEnd");
}

void AtomSandboxedRendererClient::DidCreateScriptContext(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) {
  RendererClientBase::DidCreateScriptContext(context, render_frame);

  // Only allow preload for the main frame or
  // For devtools we still want to run the preload_bundle script
  // Or when nodeSupport is explicitly enabled in sub frames
  bool is_main_frame = render_frame->IsMainFrame();
  bool is_devtools =
      IsDevTools(render_frame) || IsDevToolsExtension(render_frame);
  bool allow_node_in_sub_frames =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNodeIntegrationInSubFrames);
  bool should_load_preload =
      (is_main_frame || is_devtools || allow_node_in_sub_frames) &&
      !IsWebViewFrame(context, render_frame);
  if (!should_load_preload)
    return;

  injected_frames_.insert(render_frame);

  // Wrap the bundle into a function that receives the binding object as
  // argument.
  auto* isolate = context->GetIsolate();
  auto binding = v8::Object::New(isolate);
  InitializeBindings(binding, context, render_frame->IsMainFrame());
  AddRenderBindings(isolate, binding);

  std::vector<v8::Local<v8::String>> sandbox_preload_bundle_params = {
      node::FIXED_ONE_BYTE_STRING(isolate, "binding")};

  std::vector<v8::Local<v8::Value>> sandbox_preload_bundle_args = {binding};

  util::CompileAndCall(
      isolate->GetCurrentContext(), "electron/js2c/sandbox_bundle",
      &sandbox_preload_bundle_params, &sandbox_preload_bundle_args, nullptr);

  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  InvokeHiddenCallback(context, kLifecycleKey, "onLoaded");
}

void AtomSandboxedRendererClient::SetupMainWorldOverrides(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) {
  // Setup window overrides in the main world context
  // Wrap the bundle into a function that receives the isolatedWorld as
  // an argument.
  auto* isolate = context->GetIsolate();

  gin_helper::Dictionary process = gin::Dictionary::CreateEmpty(isolate);
  process.SetMethod("_linkedBinding", GetBinding);

  std::vector<v8::Local<v8::String>> isolated_bundle_params = {
      node::FIXED_ONE_BYTE_STRING(isolate, "nodeProcess"),
      node::FIXED_ONE_BYTE_STRING(isolate, "isolatedWorld")};

  std::vector<v8::Local<v8::Value>> isolated_bundle_args = {
      process.GetHandle(),
      GetContext(render_frame->GetWebFrame(), isolate)->Global()};

  util::CompileAndCall(context, "electron/js2c/isolated_bundle",
                       &isolated_bundle_params, &isolated_bundle_args, nullptr);
}

void AtomSandboxedRendererClient::SetupExtensionWorldOverrides(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame,
    int world_id) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  NOTREACHED();
#else
  auto* isolate = context->GetIsolate();

  gin_helper::Dictionary process = gin::Dictionary::CreateEmpty(isolate);
  process.SetMethod("_linkedBinding", GetBinding);

  std::vector<v8::Local<v8::String>> isolated_bundle_params = {
      node::FIXED_ONE_BYTE_STRING(isolate, "nodeProcess"),
      node::FIXED_ONE_BYTE_STRING(isolate, "isolatedWorld"),
      node::FIXED_ONE_BYTE_STRING(isolate, "worldId")};

  std::vector<v8::Local<v8::Value>> isolated_bundle_args = {
      process.GetHandle(),
      GetContext(render_frame->GetWebFrame(), isolate)->Global(),
      v8::Integer::New(isolate, world_id)};

  util::CompileAndCall(context, "electron/js2c/content_script_bundle",
                       &isolated_bundle_params, &isolated_bundle_args, nullptr);
#endif
}

void AtomSandboxedRendererClient::WillReleaseScriptContext(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) {
  if (injected_frames_.erase(render_frame) == 0)
    return;

  auto* isolate = context->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  InvokeHiddenCallback(context, kLifecycleKey, "onExit");
}

}  // namespace electron
