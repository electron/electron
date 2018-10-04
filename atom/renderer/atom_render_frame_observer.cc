// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_render_frame_observer.h"

#include <string>
#include <vector>

#include "atom/common/api/api_messages.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/heap_snapshot.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_event.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_message_macros.h"
#include "native_mate/dictionary.h"
#include "net/base/net_module.h"
#include "net/grit/net_resources.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_draggable_region.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"

namespace atom {

namespace {

bool GetIPCObject(v8::Isolate* isolate,
                  v8::Local<v8::Context> context,
                  v8::Local<v8::Object>* ipc) {
  v8::Local<v8::String> key = mate::StringToV8(isolate, "ipc");
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
  v8::Local<v8::Object> global_object = context->Global();
  v8::Local<v8::Value> value;
  if (!global_object->GetPrivate(context, privateKey).ToLocal(&value))
    return false;
  if (value.IsEmpty() || !value->IsObject())
    return false;
  *ipc = value->ToObject();
  return true;
}

std::vector<v8::Local<v8::Value>> ListValueToVector(
    v8::Isolate* isolate,
    const base::ListValue& list) {
  v8::Local<v8::Value> array = mate::ConvertToV8(isolate, list);
  std::vector<v8::Local<v8::Value>> result;
  mate::ConvertFromV8(isolate, array, &result);
  return result;
}

base::StringPiece NetResourceProvider(int key) {
  if (key == IDR_DIR_HEADER_HTML) {
    base::StringPiece html_data =
        ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
            IDR_DIR_HEADER_HTML);
    return html_data;
  }
  return base::StringPiece();
}

}  // namespace

AtomRenderFrameObserver::AtomRenderFrameObserver(
    content::RenderFrame* frame,
    RendererClientBase* renderer_client)
    : content::RenderFrameObserver(frame),
      render_frame_(frame),
      renderer_client_(renderer_client) {
  // Initialise resource for directory listing.
  net::NetModule::SetResourceProvider(NetResourceProvider);
}

void AtomRenderFrameObserver::DidClearWindowObject() {
  renderer_client_->DidClearWindowObject(render_frame_);
}

void AtomRenderFrameObserver::DidCreateDocumentElement() {
  document_created_ = true;
}

void AtomRenderFrameObserver::DidCreateScriptContext(
    v8::Handle<v8::Context> context,
    int world_id) {
  if (ShouldNotifyClient(world_id))
    renderer_client_->DidCreateScriptContext(context, render_frame_);

  if (renderer_client_->isolated_world() && IsMainWorld(world_id) &&
      render_frame_->IsMainFrame()) {
    CreateIsolatedWorldContext();
    renderer_client_->SetupMainWorldOverrides(context);
  }
}

void AtomRenderFrameObserver::DraggableRegionsChanged() {
  blink::WebVector<blink::WebDraggableRegion> webregions =
      render_frame_->GetWebFrame()->GetDocument().DraggableRegions();
  std::vector<DraggableRegion> regions;
  for (auto& webregion : webregions) {
    DraggableRegion region;
    render_frame_->GetRenderView()->ConvertViewportToWindowViaWidget(
        &webregion.bounds);
    region.bounds = webregion.bounds;
    region.draggable = webregion.draggable;
    regions.push_back(region);
  }
  Send(new AtomFrameHostMsg_UpdateDraggableRegions(routing_id(), regions));
}

void AtomRenderFrameObserver::WillReleaseScriptContext(
    v8::Local<v8::Context> context,
    int world_id) {
  if (ShouldNotifyClient(world_id))
    renderer_client_->WillReleaseScriptContext(context, render_frame_);
}

void AtomRenderFrameObserver::OnDestruct() {
  delete this;
}

void AtomRenderFrameObserver::CreateIsolatedWorldContext() {
  auto* frame = render_frame_->GetWebFrame();

  // This maps to the name shown in the context combo box in the Console tab
  // of the dev tools.
  frame->SetIsolatedWorldHumanReadableName(
      World::ISOLATED_WORLD,
      blink::WebString::FromUTF8("Electron Isolated Context"));

  // Setup document's origin policy in isolated world
  frame->SetIsolatedWorldSecurityOrigin(
      World::ISOLATED_WORLD, frame->GetDocument().GetSecurityOrigin());

  // Create initial script context in isolated world
  blink::WebScriptSource source("void 0");
  frame->ExecuteScriptInIsolatedWorld(World::ISOLATED_WORLD, &source, 1);
}

bool AtomRenderFrameObserver::IsMainWorld(int world_id) {
  return world_id == World::MAIN_WORLD;
}

bool AtomRenderFrameObserver::IsIsolatedWorld(int world_id) {
  return world_id == World::ISOLATED_WORLD;
}

bool AtomRenderFrameObserver::ShouldNotifyClient(int world_id) {
  if (renderer_client_->isolated_world() && render_frame_->IsMainFrame())
    return IsIsolatedWorld(world_id);
  else
    return IsMainWorld(world_id);
}

bool AtomRenderFrameObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AtomRenderFrameObserver, message)
    IPC_MESSAGE_HANDLER(AtomFrameMsg_Message, OnBrowserMessage)
    IPC_MESSAGE_HANDLER(AtomFrameMsg_TakeHeapSnapshot, OnTakeHeapSnapshot)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AtomRenderFrameObserver::OnBrowserMessage(bool send_to_all,
                                               const std::string& channel,
                                               const base::ListValue& args,
                                               int32_t sender_id) {
  // Don't handle browser messages before document element is created.
  // When we receive a message from the browser, we try to transfer it
  // to a web page, and when we do that Blink creates an empty
  // document element if it hasn't been created yet, and it makes our init
  // script to run while `window.location` is still "about:blank".
  if (!document_created_)
    return;

  blink::WebLocalFrame* frame = render_frame_->GetWebFrame();
  if (!frame || !render_frame_->IsMainFrame())
    return;

  EmitIPCEvent(frame, channel, args, sender_id);

  // Also send the message to all sub-frames.
  if (send_to_all) {
    for (blink::WebFrame* child = frame->FirstChild(); child;
         child = child->NextSibling())
      if (child->IsWebLocalFrame()) {
        EmitIPCEvent(child->ToWebLocalFrame(), channel, args, sender_id);
      }
  }
}

void AtomRenderFrameObserver::OnTakeHeapSnapshot(
    IPC::PlatformFileForTransit file_handle,
    const std::string& channel) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;

  auto file = IPC::PlatformFileForTransitToFile(file_handle);
  bool success = TakeHeapSnapshot(blink::MainThreadIsolate(), &file);

  base::ListValue args;
  args.AppendString(channel);
  args.AppendBoolean(success);

  render_frame_->Send(new AtomFrameHostMsg_Message(
      render_frame_->GetRoutingID(), "ipc-message", args));
}

void AtomRenderFrameObserver::EmitIPCEvent(blink::WebLocalFrame* frame,
                                           const std::string& channel,
                                           const base::ListValue& args,
                                           int32_t sender_id) {
  if (!frame)
    return;

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = renderer_client_->GetContext(frame, isolate);
  v8::Context::Scope context_scope(context);

  // Only emit IPC event for context with node integration.
  node::Environment* env = node::Environment::GetCurrent(context);
  if (!env)
    return;

  v8::Local<v8::Object> ipc;
  if (GetIPCObject(isolate, context, &ipc)) {
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

}  // namespace atom
