// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/renderer/electron_api_service_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/environment.h"
#include "base/macros.h"
#include "base/threading/thread_restrictions.h"
#include "electron/shell/common/api/event_emitter_caller.h"
#include "electron/shell/common/node_includes.h"
#include "electron/shell/common/options_switches.h"
#include "electron/shell/renderer/atom_render_frame_observer.h"
#include "electron/shell/renderer/renderer_client_base.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "native_mate/dictionary.h"
#include "shell/common/atom_constants.h"
#include "shell/common/heap_snapshot.h"
#include "shell/common/native_mate_converters/value_converter.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

namespace {

const char kIpcKey[] = "ipcNative";

// Gets the private object under kIpcKey
v8::Local<v8::Object> GetIpcObject(v8::Local<v8::Context> context) {
  auto* isolate = context->GetIsolate();
  auto binding_key =
      mate::ConvertToV8(isolate, kIpcKey)->ToString(context).ToLocalChecked();
  auto private_binding_key = v8::Private::ForApi(isolate, binding_key);
  auto global_object = context->Global();
  auto value =
      global_object->GetPrivate(context, private_binding_key).ToLocalChecked();
  DCHECK(!value.IsEmpty() && value->IsObject());
  return value->ToObject(context).ToLocalChecked();
}

void InvokeIpcCallback(v8::Local<v8::Context> context,
                       const std::string& callback_name,
                       std::vector<v8::Local<v8::Value>> args) {
  TRACE_EVENT0("devtools.timeline", "FunctionCall");
  auto* isolate = context->GetIsolate();

  auto ipcNative = GetIpcObject(context);

  // Only set up the node::CallbackScope if there's a node environment.
  // Sandboxed renderers don't have a node environment.
  node::Environment* env = node::Environment::GetCurrent(context);
  std::unique_ptr<node::CallbackScope> callback_scope;
  if (env) {
    callback_scope.reset(new node::CallbackScope(isolate, ipcNative, {0, 0}));
  }

  auto callback_key = mate::ConvertToV8(isolate, callback_name)
                          ->ToString(context)
                          .ToLocalChecked();
  auto callback_value = ipcNative->Get(context, callback_key).ToLocalChecked();
  DCHECK(callback_value->IsFunction());  // set by init.ts
  auto callback = v8::Local<v8::Function>::Cast(callback_value);
  ignore_result(callback->Call(context, ipcNative, args.size(), args.data()));
}

void EmitIPCEvent(v8::Local<v8::Context> context,
                  bool internal,
                  const std::string& channel,
                  const std::vector<base::Value>& args,
                  int32_t sender_id) {
  auto* isolate = context->GetIsolate();

  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope script_scope(isolate,
                                   v8::MicrotasksScope::kRunMicrotasks);

  std::vector<v8::Local<v8::Value>> argv = {
      mate::ConvertToV8(isolate, internal), mate::ConvertToV8(isolate, channel),
      mate::ConvertToV8(isolate, args), mate::ConvertToV8(isolate, sender_id)};

  InvokeIpcCallback(context, "onMessage", argv);
}

}  // namespace

ElectronApiServiceImpl::~ElectronApiServiceImpl() = default;

ElectronApiServiceImpl::ElectronApiServiceImpl(
    content::RenderFrame* render_frame,
    RendererClientBase* renderer_client,
    mojom::ElectronRendererAssociatedRequest request)
    : content::RenderFrameObserver(render_frame),
      binding_(this),
      render_frame_(render_frame),
      renderer_client_(renderer_client) {
  binding_.Bind(std::move(request));
  binding_.set_connection_error_handler(base::BindOnce(
      &ElectronApiServiceImpl::OnDestruct, base::Unretained(this)));
}

// static
void ElectronApiServiceImpl::CreateMojoService(
    content::RenderFrame* render_frame,
    RendererClientBase* renderer_client,
    mojom::ElectronRendererAssociatedRequest request) {
  DCHECK(render_frame);

  // Owns itself. Will be deleted when the render frame is destroyed.
  new ElectronApiServiceImpl(render_frame, renderer_client, std::move(request));
}

void ElectronApiServiceImpl::OnDestruct() {
  delete this;
}

void ElectronApiServiceImpl::Message(bool internal,
                                     bool send_to_all,
                                     const std::string& channel,
                                     base::Value arguments,
                                     int32_t sender_id) {
  blink::WebLocalFrame* frame = render_frame_->GetWebFrame();
  if (!frame)
    return;

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = renderer_client_->GetContext(frame, isolate);

  EmitIPCEvent(context, internal, channel, arguments.GetList(), sender_id);

  // Also send the message to all sub-frames.
  // TODO(MarshallOfSound): Completely move this logic to the main process
  if (send_to_all) {
    for (blink::WebFrame* child = frame->FirstChild(); child;
         child = child->NextSibling())
      if (child->IsWebLocalFrame()) {
        v8::Local<v8::Context> child_context =
            renderer_client_->GetContext(child->ToWebLocalFrame(), isolate);
        EmitIPCEvent(child_context, internal, channel, arguments.GetList(),
                     sender_id);
      }
  }
}

void ElectronApiServiceImpl::UpdateCrashpadPipeName(
    const std::string& pipe_name) {
#if defined(OS_WIN)
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  env->SetVar(kCrashpadPipeName, pipe_name);
#endif
}

void ElectronApiServiceImpl::TakeHeapSnapshot(
    mojo::ScopedHandle file,
    TakeHeapSnapshotCallback callback) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;

  base::PlatformFile platform_file;
  if (mojo::UnwrapPlatformFile(std::move(file), &platform_file) !=
      MOJO_RESULT_OK) {
    LOG(ERROR) << "Unable to get the file handle from mojo.";
    std::move(callback).Run(false);
    return;
  }
  base::File base_file(platform_file);

  bool success =
      electron::TakeHeapSnapshot(blink::MainThreadIsolate(), &base_file);

  std::move(callback).Run(success);
}

}  // namespace electron
