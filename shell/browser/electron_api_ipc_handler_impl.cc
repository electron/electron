// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_api_ipc_handler_impl.h"

#include <utility>

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "gin/handle.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_helper/event.h"

namespace electron {
ElectronApiIPCHandlerImpl::ElectronApiIPCHandlerImpl(
    content::RenderFrameHost* frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronApiIPC> receiver)
    : render_frame_host_id_(frame_host->GetGlobalId()) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host);
  DCHECK(web_contents);
  content::WebContentsObserver::Observe(web_contents);

  receiver_.Bind(std::move(receiver));
  receiver_.set_disconnect_handler(base::BindOnce(
      &ElectronApiIPCHandlerImpl::OnConnectionError, GetWeakPtr()));
}

ElectronApiIPCHandlerImpl::~ElectronApiIPCHandlerImpl() = default;

void ElectronApiIPCHandlerImpl::WebContentsDestroyed() {
  delete this;
}

void ElectronApiIPCHandlerImpl::OnConnectionError() {
  delete this;
}

void ElectronApiIPCHandlerImpl::Message(bool internal,
                                        const std::string& channel,
                                        blink::CloneableMessage arguments) {
  auto* session = GetSession();
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto event = MakeIPCEvent(isolate, session, internal);
  if (event.IsEmpty())
    return;
  session->Message(event, channel, std::move(arguments));
}
void ElectronApiIPCHandlerImpl::Invoke(bool internal,
                                       const std::string& channel,
                                       blink::CloneableMessage arguments,
                                       InvokeCallback callback) {
  auto* session = GetSession();
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto event = MakeIPCEvent(isolate, session, internal, std::move(callback));
  if (event.IsEmpty())
    return;
  session->Invoke(event, channel, std::move(arguments));
}

void ElectronApiIPCHandlerImpl::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message) {
  auto* session = GetSession();
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto event = MakeIPCEvent(isolate, session, false);
  if (event.IsEmpty())
    return;
  session->ReceivePostMessage(event, channel, std::move(message));
}

void ElectronApiIPCHandlerImpl::MessageSync(bool internal,
                                            const std::string& channel,
                                            blink::CloneableMessage arguments,
                                            MessageSyncCallback callback) {
  auto* session = GetSession();
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto event = MakeIPCEvent(isolate, session, internal, std::move(callback));
  if (event.IsEmpty())
    return;
  session->MessageSync(event, channel, std::move(arguments));
}

void ElectronApiIPCHandlerImpl::MessageHost(const std::string& channel,
                                            blink::CloneableMessage arguments) {
  auto* session = GetSession();
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto event = MakeIPCEvent(isolate, session, false);
  if (event.IsEmpty())
    return;
  session->MessageHost(event, channel, std::move(arguments));
}

content::RenderFrameHost* ElectronApiIPCHandlerImpl::GetRenderFrameHost() {
  return content::RenderFrameHost::FromID(render_frame_host_id_);
}

api::Session* ElectronApiIPCHandlerImpl::GetSession() {
  auto* rfh = GetRenderFrameHost();
  return rfh ? api::Session::FromBrowserContext(rfh->GetBrowserContext())
             : nullptr;
}

gin::Handle<gin_helper::internal::Event>
ElectronApiIPCHandlerImpl::MakeIPCEvent(
    v8::Isolate* isolate,
    api::Session* session,
    bool internal,
    electron::mojom::ElectronApiIPC::InvokeCallback callback) {
  if (!session) {
    if (callback) {
      // We must always invoke the callback if present.
      gin_helper::internal::ReplyChannel::Create(isolate, std::move(callback))
          ->SendError("Session does not exist");
    }
    return {};
  }

  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (!api_web_contents) {
    if (callback) {
      // We must always invoke the callback if present.
      gin_helper::internal::ReplyChannel::Create(isolate, std::move(callback))
          ->SendError("WebContents does not exist");
    }
    return {};
  }

  v8::Local<v8::Object> wrapper;
  if (!api_web_contents->GetWrapper(isolate).ToLocal(&wrapper)) {
    if (callback) {
      // We must always invoke the callback if present.
      gin_helper::internal::ReplyChannel::Create(isolate, std::move(callback))
          ->SendError("WebContents was destroyed");
    }
    return {};
  }

  content::RenderFrameHost* frame = GetRenderFrameHost();
  gin::Handle<gin_helper::internal::Event> event =
      gin_helper::internal::Event::New(isolate);
  gin_helper::Dictionary dict(isolate, event.ToV8().As<v8::Object>());
  dict.Set("type", "frame");
  dict.Set("sender", web_contents());
  if (internal)
    dict.SetHidden("internal", internal);
  if (callback)
    dict.Set("_replyChannel", gin_helper::internal::ReplyChannel::Create(
                                  isolate, std::move(callback)));
  if (frame) {
    dict.SetGetter("senderFrame", frame);
    dict.Set("frameId", frame->GetRoutingID());
    dict.Set("processId", frame->GetProcess()->GetID().GetUnsafeValue());
    dict.Set("frameTreeNodeId", frame->GetFrameTreeNodeId());
  }
  return event;
}

// static
void ElectronApiIPCHandlerImpl::Create(
    content::RenderFrameHost* frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronApiIPC> receiver) {
  new ElectronApiIPCHandlerImpl(frame_host, std::move(receiver));
}
}  // namespace electron
