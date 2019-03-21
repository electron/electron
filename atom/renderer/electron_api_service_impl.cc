// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/atom/renderer/electron_api_service_impl.h"

#include <utility>
#include <vector>

#include "atom/common/heap_snapshot.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/macros.h"
#include "base/threading/thread_restrictions.h"
#include "electron/atom/common/api/event_emitter_caller.h"
#include "electron/atom/common/node_includes.h"
#include "electron/atom/common/options_switches.h"
#include "electron/atom/renderer/atom_render_frame_observer.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "native_mate/dictionary.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace atom {

namespace {

const char kIpcKey[] = "ipcNative";

void InvokeIpcCallback(v8::Handle<v8::Context> context,
                       const std::string& callback_name,
                       std::vector<v8::Handle<v8::Value>> args) {
  TRACE_EVENT0("devtools.timeline", "FunctionCall");
  auto* isolate = context->GetIsolate();
  auto binding_key = mate::ConvertToV8(isolate, kIpcKey)->ToString(isolate);
  auto private_binding_key = v8::Private::ForApi(isolate, binding_key);
  auto global_object = context->Global();
  v8::Local<v8::Value> value;
  if (!global_object->GetPrivate(context, private_binding_key).ToLocal(&value))
    return;
  if (value.IsEmpty() || !value->IsObject())
    return;
  auto binding = value->ToObject(isolate);
  auto callback_key =
      mate::ConvertToV8(isolate, callback_name)->ToString(isolate);
  auto callback_value = binding->Get(callback_key);
  DCHECK(callback_value->IsFunction());  // set by init.ts
  auto callback = v8::Handle<v8::Function>::Cast(callback_value);
  ignore_result(callback->Call(context, binding, args.size(), args.data()));
}

void EmitIPCEvent(blink::WebLocalFrame* frame,
                  bool isolated_world,
                  bool internal,
                  const std::string& channel,
                  const std::vector<base::Value>& args,
                  int32_t sender_id) {
  if (!frame)
    return;

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context =
      isolated_world ? frame->WorldScriptContext(isolate, World::ISOLATED_WORLD)
                     : frame->MainWorldScriptContext();
  v8::Context::Scope context_scope(context);

  std::vector<v8::Local<v8::Value>> argv = {
      mate::ConvertToV8(isolate, internal), mate::ConvertToV8(isolate, channel),
      mate::ConvertToV8(isolate, args), mate::ConvertToV8(isolate, sender_id)};

  InvokeIpcCallback(context, "onMessage", argv);
}

}  // namespace

ElectronApiServiceImpl::~ElectronApiServiceImpl() = default;

ElectronApiServiceImpl::ElectronApiServiceImpl(
    content::RenderFrame* render_frame,
    mojom::ElectronRendererAssociatedRequest request)
    : content::RenderFrameObserver(render_frame),
      binding_(this),
      render_frame_(render_frame) {
  isolated_world_ = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kContextIsolation);
  binding_.Bind(std::move(request));
  binding_.set_connection_error_handler(base::BindOnce(
      &ElectronApiServiceImpl::OnDestruct, base::Unretained(this)));
}

// static
void ElectronApiServiceImpl::CreateMojoService(
    content::RenderFrame* render_frame,
    mojom::ElectronRendererAssociatedRequest request) {
  DCHECK(render_frame);

  // Owns itself. Will be deleted when the render frame is destroyed.
  new ElectronApiServiceImpl(render_frame, std::move(request));
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

  EmitIPCEvent(frame, isolated_world_, internal, channel, arguments.GetList(),
               sender_id);

  // Also send the message to all sub-frames.
  if (send_to_all) {
    for (blink::WebFrame* child = frame->FirstChild(); child;
         child = child->NextSibling())
      if (child->IsWebLocalFrame()) {
        EmitIPCEvent(child->ToWebLocalFrame(), isolated_world_, internal,
                     channel, arguments.GetList(), sender_id);
      }
  }
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

  bool success = atom::TakeHeapSnapshot(blink::MainThreadIsolate(), &base_file);

  std::move(callback).Run(success);
}

}  // namespace atom
