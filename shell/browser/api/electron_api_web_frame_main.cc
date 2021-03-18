// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_frame_main.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "content/browser/find_request_manager.h"            // nogncheck
#include "content/browser/renderer_host/frame_tree_node.h"   // nogncheck
#include "content/browser/web_contents/web_contents_impl.h"  // nogncheck
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "electron/shell/common/api/api.mojom.h"
#include "gin/object_template_builder.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/browser/api/message_port.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_value_serializer.h"
#include "third_party/blink/public/mojom/frame/find_in_page.mojom.h"

namespace electron {

namespace api {

typedef std::unordered_map<content::RenderFrameHost*, WebFrameMain*>
    RenderFrameMap;
base::LazyInstance<RenderFrameMap>::DestructorAtExit g_render_frame_map =
    LAZY_INSTANCE_INITIALIZER;

WebFrameMain* FromRenderFrameHost(content::RenderFrameHost* rfh) {
  auto frame_map = g_render_frame_map.Get();
  auto iter = frame_map.find(rfh);
  auto* web_frame = iter == frame_map.end() ? nullptr : iter->second;
  return web_frame;
}

gin::WrapperInfo WebFrameMain::kWrapperInfo = {gin::kEmbedderNativeGin};

WebFrameMain::WebFrameMain(content::RenderFrameHost* rfh) : render_frame_(rfh) {
  g_render_frame_map.Get().emplace(rfh, this);
}

WebFrameMain::~WebFrameMain() {
  MarkRenderFrameDisposed();
}

void WebFrameMain::MarkRenderFrameDisposed() {
  if (render_frame_disposed_)
    return;
  Unpin();
  g_render_frame_map.Get().erase(render_frame_);
  render_frame_disposed_ = true;
}

bool WebFrameMain::CheckRenderFrame() const {
  if (render_frame_disposed_) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::Locker locker(isolate);
    v8::HandleScope scope(isolate);
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Render frame was disposed before WebFrameMain could be accessed");
    return false;
  }
  return true;
}

v8::Local<v8::Promise> WebFrameMain::ExecuteJavaScript(
    gin::Arguments* args,
    const base::string16& code) {
  gin_helper::Promise<base::Value> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // Optional userGesture parameter
  bool user_gesture;
  if (!args->PeekNext().IsEmpty()) {
    if (args->PeekNext()->IsBoolean()) {
      args->GetNext(&user_gesture);
    } else {
      args->ThrowTypeError("userGesture must be a boolean");
      return handle;
    }
  } else {
    user_gesture = false;
  }

  if (render_frame_disposed_) {
    promise.RejectWithErrorMessage(
        "Render frame was disposed before WebFrameMain could be accessed");
    return handle;
  }

  if (user_gesture) {
    auto* ftn = content::FrameTreeNode::From(render_frame_);
    ftn->UpdateUserActivationState(
        blink::mojom::UserActivationUpdateType::kNotifyActivation,
        blink::mojom::UserActivationNotificationType::kTest);
  }

  render_frame_->ExecuteJavaScriptForTests(
      code, base::BindOnce([](gin_helper::Promise<base::Value> promise,
                              base::Value value) { promise.Resolve(value); },
                           std::move(promise)));

  return handle;
}

bool WebFrameMain::Reload() {
  if (!CheckRenderFrame())
    return false;
  return render_frame_->Reload();
}

uint32_t WebFrameMain::FindInFrame(gin::Arguments* args) {
  base::string16 search_text;
  if (!args->GetNext(&search_text) || search_text.empty()) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("Must provide a non-empty search content");
    return 0;
  }

  if (!CheckRenderFrame())
    return 0;

  uint32_t request_id = ++find_in_frame_request_id_;
  gin_helper::Dictionary dict;
  auto options = blink::mojom::FindOptions::New();
  if (args->GetNext(&dict)) {
    dict.Get("forward", &options->forward);
    dict.Get("matchCase", &options->match_case);
    dict.Get("findNext", &options->new_session);
  }

  GetOrCreateFindRequestManager()->Find(request_id, search_text,
                                        std::move(options));
  return request_id;
}

void WebFrameMain::StopFindInFrame(content::StopFindAction action) {
  if (CheckRenderFrame() && find_request_manager_)
    find_request_manager_->StopFinding(action);
}

void WebFrameMain::Send(v8::Isolate* isolate,
                        bool internal,
                        const std::string& channel,
                        v8::Local<v8::Value> args) {
  blink::CloneableMessage message;
  if (!gin::ConvertFromV8(isolate, args, &message)) {
    isolate->ThrowException(v8::Exception::Error(
        gin::StringToV8(isolate, "Failed to serialize arguments")));
    return;
  }

  if (!CheckRenderFrame())
    return;

  GetRendererApi()->Message(internal, channel, std::move(message),
                            0 /* sender_id */);
}

const mojo::Remote<mojom::ElectronRenderer>& WebFrameMain::GetRendererApi() {
  if (!renderer_api_) {
    pending_receiver_ = renderer_api_.BindNewPipeAndPassReceiver();
    if (render_frame_->IsRenderFrameCreated()) {
      render_frame_->GetRemoteInterfaces()->GetInterface(
          std::move(pending_receiver_));
    }
    renderer_api_.set_disconnect_handler(base::BindOnce(
        &WebFrameMain::OnRendererConnectionError, weak_factory_.GetWeakPtr()));
  }
  return renderer_api_;
}

void WebFrameMain::OnRendererConnectionError() {
  renderer_api_.reset();
}

void WebFrameMain::PostMessage(v8::Isolate* isolate,
                               const std::string& channel,
                               v8::Local<v8::Value> message_value,
                               base::Optional<v8::Local<v8::Value>> transfer) {
  blink::TransferableMessage transferable_message;
  if (!electron::SerializeV8Value(isolate, message_value,
                                  &transferable_message)) {
    // SerializeV8Value sets an exception.
    return;
  }

  std::vector<gin::Handle<MessagePort>> wrapped_ports;
  if (transfer) {
    if (!gin::ConvertFromV8(isolate, *transfer, &wrapped_ports)) {
      isolate->ThrowException(v8::Exception::Error(
          gin::StringToV8(isolate, "Invalid value for transfer")));
      return;
    }
  }

  bool threw_exception = false;
  transferable_message.ports =
      MessagePort::DisentanglePorts(isolate, wrapped_ports, &threw_exception);
  if (threw_exception)
    return;

  if (!CheckRenderFrame())
    return;

  GetRendererApi()->ReceivePostMessage(channel,
                                       std::move(transferable_message));
}

int WebFrameMain::FrameTreeNodeID() const {
  if (!CheckRenderFrame())
    return -1;
  return render_frame_->GetFrameTreeNodeId();
}

std::string WebFrameMain::Name() const {
  if (!CheckRenderFrame())
    return std::string();
  return render_frame_->GetFrameName();
}

base::ProcessId WebFrameMain::OSProcessID() const {
  if (!CheckRenderFrame())
    return -1;
  base::ProcessHandle process_handle =
      render_frame_->GetProcess()->GetProcess().Handle();
  return base::GetProcId(process_handle);
}

int WebFrameMain::ProcessID() const {
  if (!CheckRenderFrame())
    return -1;
  return render_frame_->GetProcess()->GetID();
}

int WebFrameMain::RoutingID() const {
  if (!CheckRenderFrame())
    return -1;
  return render_frame_->GetRoutingID();
}

GURL WebFrameMain::URL() const {
  if (!CheckRenderFrame())
    return GURL::EmptyGURL();
  return render_frame_->GetLastCommittedURL();
}

content::RenderFrameHost* WebFrameMain::Top() const {
  if (!CheckRenderFrame())
    return nullptr;
  return render_frame_->GetMainFrame();
}

content::RenderFrameHost* WebFrameMain::Parent() const {
  if (!CheckRenderFrame())
    return nullptr;
  return render_frame_->GetParent();
}

std::vector<content::RenderFrameHost*> WebFrameMain::Frames() const {
  std::vector<content::RenderFrameHost*> frame_hosts;
  if (!CheckRenderFrame())
    return frame_hosts;

  for (auto* rfh : render_frame_->GetFramesInSubtree()) {
    if (rfh->GetParent() == render_frame_)
      frame_hosts.push_back(rfh);
  }

  return frame_hosts;
}

std::vector<content::RenderFrameHost*> WebFrameMain::FramesInSubtree() const {
  std::vector<content::RenderFrameHost*> frame_hosts;
  if (!CheckRenderFrame())
    return frame_hosts;

  for (auto* rfh : render_frame_->GetFramesInSubtree()) {
    frame_hosts.push_back(rfh);
  }

  return frame_hosts;
}

// static
gin::Handle<WebFrameMain> WebFrameMain::New(v8::Isolate* isolate) {
  return gin::Handle<WebFrameMain>();
}

// static
gin::Handle<WebFrameMain> WebFrameMain::From(v8::Isolate* isolate,
                                             content::RenderFrameHost* rfh) {
  if (rfh == nullptr)
    return gin::Handle<WebFrameMain>();
  auto* web_frame = FromRenderFrameHost(rfh);
  if (web_frame)
    return gin::CreateHandle(isolate, web_frame);

  auto handle = gin::CreateHandle(isolate, new WebFrameMain(rfh));

  // Prevent garbage collection of frame until it has been deleted internally.
  handle->Pin(isolate);

  return handle;
}

// static
gin::Handle<WebFrameMain> WebFrameMain::FromID(v8::Isolate* isolate,
                                               int render_process_id,
                                               int render_frame_id) {
  auto* rfh =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  return From(isolate, rfh);
}

// static
void WebFrameMain::RenderFrameDeleted(content::RenderFrameHost* rfh) {
  auto* web_frame = FromRenderFrameHost(rfh);
  if (web_frame)
    web_frame->MarkRenderFrameDisposed();
}

void WebFrameMain::RenderFrameCreated(content::RenderFrameHost* rfh) {
  auto* web_frame = FromRenderFrameHost(rfh);
  if (web_frame)
    web_frame->Connect();
}

void WebFrameMain::Connect() {
  if (pending_receiver_) {
    render_frame_->GetRemoteInterfaces()->GetInterface(
        std::move(pending_receiver_));
  }
}

content::FindRequestManager* WebFrameMain::GetOrCreateFindRequestManager() {
  if (!find_request_manager_.get()) {
    // No existing FindRequestManager found, so one must be created.
    auto* web_contents = static_cast<content::WebContentsImpl*>(
        content::WebContents::FromRenderFrameHost(render_frame_));
    find_request_manager_ = std::make_unique<content::FindRequestManager>(
        web_contents,
        static_cast<content::RenderFrameHostImpl*>(render_frame_));
  }
  // Concurrent find sessions must not overlap, so destroy any existing
  // FindRequestManagers in any inner WebFrameMain.
  for (content::RenderFrameHost* rfh :
       render_frame_->GetMainFrame()->GetFramesInSubtree()) {
    if (rfh == render_frame_)
      continue;
    auto* web_frame_main = FromRenderFrameHost(rfh);
    if (web_frame_main && web_frame_main->find_request_manager_) {
      web_frame_main->find_request_manager_->StopFinding(
          content::STOP_FIND_ACTION_CLEAR_SELECTION);
      web_frame_main->find_request_manager_.release();
    }
  }
  return find_request_manager_.get();
}

// static
v8::Local<v8::ObjectTemplate> WebFrameMain::FillObjectTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ) {
  return gin_helper::ObjectTemplateBuilder(isolate, templ)
      .SetMethod("executeJavaScript", &WebFrameMain::ExecuteJavaScript)
      .SetMethod("reload", &WebFrameMain::Reload)
      .SetMethod("findInFrame", &WebFrameMain::FindInFrame)
      .SetMethod("stopFindInFrame", &WebFrameMain::StopFindInFrame)
      .SetMethod("_send", &WebFrameMain::Send)
      .SetMethod("_postMessage", &WebFrameMain::PostMessage)
      .SetProperty("frameTreeNodeId", &WebFrameMain::FrameTreeNodeID)
      .SetProperty("name", &WebFrameMain::Name)
      .SetProperty("osProcessId", &WebFrameMain::OSProcessID)
      .SetProperty("processId", &WebFrameMain::ProcessID)
      .SetProperty("routingId", &WebFrameMain::RoutingID)
      .SetProperty("url", &WebFrameMain::URL)
      .SetProperty("top", &WebFrameMain::Top)
      .SetProperty("parent", &WebFrameMain::Parent)
      .SetProperty("frames", &WebFrameMain::Frames)
      .SetProperty("framesInSubtree", &WebFrameMain::FramesInSubtree)
      .Build();
}

const char* WebFrameMain::GetTypeName() {
  return "WebFrameMain";
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::WebFrameMain;

v8::Local<v8::Value> FromID(gin_helper::ErrorThrower thrower,
                            int render_process_id,
                            int render_frame_id) {
  if (!electron::Browser::Get()->is_ready()) {
    thrower.ThrowError("WebFrameMain is available only after app ready");
    return v8::Null(thrower.isolate());
  }

  return WebFrameMain::FromID(thrower.isolate(), render_process_id,
                              render_frame_id)
      .ToV8();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("WebFrameMain", WebFrameMain::GetConstructor(context));
  dict.SetMethod("fromId", &FromID);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_web_frame_main, Initialize)
