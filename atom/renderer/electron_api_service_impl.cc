// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/atom/renderer/electron_api_service_impl.h"

#include <utility>
#include <vector>

#include "atom/common/native_mate_converters/value_converter.h"
#include "electron/atom/common/api/event_emitter_caller.h"
#include "electron/atom/common/node_includes.h"
#include "electron/atom/common/options_switches.h"
#include "electron/atom/renderer/atom_render_frame_observer.h"
#include "native_mate/dictionary.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace atom {

namespace {

std::vector<v8::Local<v8::Value>> ListValueToVector(
    v8::Isolate* isolate,
    const std::vector<base::Value>& list) {
  v8::Local<v8::Value> array = mate::ConvertToV8(isolate, list);
  std::vector<v8::Local<v8::Value>> result;
  mate::ConvertFromV8(isolate, array, &result);
  return result;
}

bool GetIPCObject(v8::Isolate* isolate,
                  v8::Local<v8::Context> context,
                  bool internal,
                  v8::Local<v8::Object>* ipc) {
  v8::Local<v8::String> key =
      mate::StringToV8(isolate, internal ? "ipc-internal" : "ipc");
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
  v8::Local<v8::Object> global_object = context->Global();
  v8::Local<v8::Value> value;
  if (!global_object->GetPrivate(context, privateKey).ToLocal(&value))
    return false;
  if (value.IsEmpty() || !value->IsObject())
    return false;
  *ipc = value->ToObject(isolate);
  return true;
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

  // Only emit IPC event for context with node integration.
  node::Environment* env = node::Environment::GetCurrent(context);
  if (!env)
    return;

  v8::Local<v8::Object> ipc;
  if (GetIPCObject(isolate, context, internal, &ipc)) {
    TRACE_EVENT0("devtools.timeline", "FunctionCall");
    auto args_vector = ListValueToVector(isolate, args);
    // Insert the Event object, event.sender is ipc.
    mate::Dictionary event = mate::Dictionary::CreateEmpty(isolate);
    event.Set("sender", ipc);
    event.Set("senderId", sender_id);
    args_vector.insert(args_vector.begin(), event.GetHandle());
    mate::EmitEvent(isolate, ipc, channel, args_vector);
  }
}

}  // namespace

ElectronApiServiceImpl::~ElectronApiServiceImpl() = default;

ElectronApiServiceImpl::ElectronApiServiceImpl(
    content::RenderFrame* render_frame,
    electron_api::mojom::ElectronRendererAssociatedRequest request)
    : content::RenderFrameObserver(render_frame),
      binding_(this),
      render_frame_(render_frame),
      weak_ptr_factory_(this) {
  (void)render_frame_;
  weak_this_ = weak_ptr_factory_.GetWeakPtr();
  isolated_world_ = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kContextIsolation);
  binding_.Bind(std::move(request));
}

// static
void ElectronApiServiceImpl::CreateMojoService(
    content::RenderFrame* render_frame,
    electron_api::mojom::ElectronRendererAssociatedRequest request) {
  DCHECK(render_frame);

  // Owns itself. Will be deleted when the render frame is destroyed.
  new ElectronApiServiceImpl(render_frame, std::move(request));
}

void ElectronApiServiceImpl::OnDestruct() {
  delete this;
}

void ElectronApiServiceImpl::Message(bool internal,
                                     const std::string& channel,
                                     base::Value arguments,
                                     int32_t sender_id) {
  /*
  // Don't handle browser messages before document element is created.
  // When we receive a message from the browser, we try to transfer it
  // to a web page, and when we do that Blink creates an empty
  // document element if it hasn't been created yet, and it makes our init
  // script to run while `window.location` is still "about:blank".
  if (!document_created_)
    return;
    */

  blink::WebLocalFrame* frame = render_frame_->GetWebFrame();
  if (!frame)
    return;

  EmitIPCEvent(frame, isolated_world_, internal, channel, arguments.GetList(),
               sender_id);

  /*
  // Also send the message to all sub-frames.
  if (send_to_all) {
    for (blink::WebFrame* child = frame->FirstChild(); child;
         child = child->NextSibling())
      if (child->IsWebLocalFrame()) {
        EmitIPCEvent(child->ToWebLocalFrame(), internal, channel, args,
                     sender_id);
      }
  }
  */
}

}  // namespace atom
