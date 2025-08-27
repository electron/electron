// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_frame_main.h"

#include <string>
#include <utility>
#include <vector>

#include "base/containers/map_util.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"  // nogncheck
#include "content/browser/renderer_host/render_process_host_impl.h"  // nogncheck
#include "content/public/browser/frame_tree_node_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/isolated_world_ids.h"
#include "gin/object_template_builder.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/browser/api/message_port.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_util.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace {

using LifecycleState = content::RenderFrameHostImpl::LifecycleStateImpl;

// RenderFrameCreated is called for speculative frames which may not be
// used in certain cross-origin navigations. Invoking
// RenderFrameHost::GetLifecycleState currently crashes when called for
// speculative frames so we need to filter it out for now. Check
// https://crbug.com/1183639 for details on when this can be removed.
[[nodiscard]] LifecycleState GetLifecycleState(
    const content::RenderFrameHost* rfh) {
  const auto* rfh_impl = static_cast<const content::RenderFrameHostImpl*>(rfh);
  return rfh_impl->lifecycle_state();
}

// RenderFrameHost (RFH) exists as a child of a FrameTreeNode. When a
// cross-origin navigation occurs, the FrameTreeNode swaps RFHs. After
// swapping, the old RFH will be marked for deletion and run any unload
// listeners. If an IPC is sent during an unload/beforeunload listener,
// it's possible that it arrives after the RFH swap and has been
// detached from the FrameTreeNode.
[[nodiscard]] bool IsDetachedFrameHost(const content::RenderFrameHost* rfh) {
  if (!rfh)
    return true;

  // During cross-origin navigation, a RFH may be swapped out of its
  // FrameTreeNode with a new RFH. In these cases, it's marked for
  // deletion. As this pending deletion RFH won't be following future
  // swaps, we need to indicate that its been detached.
  return GetLifecycleState(rfh) == LifecycleState::kRunningUnloadHandlers;
}

}  // namespace

namespace gin {

template <>
struct Converter<blink::mojom::PageVisibilityState> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   blink::mojom::PageVisibilityState val) {
    std::string visibility;
    switch (val) {
      case blink::mojom::PageVisibilityState::kVisible:
        visibility = "visible";
        break;
      case blink::mojom::PageVisibilityState::kHidden:
      case blink::mojom::PageVisibilityState::kHiddenButPainting:
        visibility = "hidden";
        break;
    }
    return gin::ConvertToV8(isolate, visibility);
  }
};

}  // namespace gin

namespace electron::api {

// FrameTreeNodeId -> WebFrameMain*
// Using FrameTreeNode allows us to track frame across navigations. This
// is most similar to how <iframe> works.
using FrameTreeNodeIdMap =
    absl::flat_hash_map<content::FrameTreeNodeId, WebFrameMain*>;

// Token -> WebFrameMain*
// Maps exact RFH to a WebFrameMain instance.
using FrameTokenMap =
    std::map<content::GlobalRenderFrameHostToken, WebFrameMain*>;

namespace {

FrameTreeNodeIdMap& GetFrameTreeNodeIdMap() {
  static base::NoDestructor<FrameTreeNodeIdMap> instance;
  return *instance;
}
FrameTokenMap& GetFrameTokenMap() {
  static base::NoDestructor<FrameTokenMap> instance;
  return *instance;
}

}  // namespace

// static
WebFrameMain* WebFrameMain::FromFrameTreeNodeId(
    content::FrameTreeNodeId frame_tree_node_id) {
  return base::FindPtrOrNull(GetFrameTreeNodeIdMap(), frame_tree_node_id);
}

// static
WebFrameMain* WebFrameMain::FromFrameToken(
    content::GlobalRenderFrameHostToken frame_token) {
  return base::FindPtrOrNull(GetFrameTokenMap(), frame_token);
}

// static
WebFrameMain* WebFrameMain::FromRenderFrameHost(content::RenderFrameHost* rfh) {
  if (!rfh)
    return nullptr;
  return FromFrameToken(rfh->GetGlobalFrameToken());
}

content::RenderFrameHost* WebFrameMain::render_frame_host() const {
  return render_frame_disposed_
             ? nullptr
             : content::RenderFrameHost::FromFrameToken(frame_token_);
}

gin::DeprecatedWrapperInfo WebFrameMain::kWrapperInfo = {
    gin::kEmbedderNativeGin};

WebFrameMain::WebFrameMain(content::RenderFrameHost* rfh)
    : frame_tree_node_id_(rfh->GetFrameTreeNodeId()),
      frame_token_(rfh->GetGlobalFrameToken()),
      render_frame_detached_(IsDetachedFrameHost(rfh)) {
  // Detached RFH should not insert itself in FTN lookup since it has been
  // swapped already.
  if (!render_frame_detached_)
    GetFrameTreeNodeIdMap().emplace(frame_tree_node_id_, this);

  const auto [_, inserted] = GetFrameTokenMap().emplace(frame_token_, this);
  DCHECK(inserted);

  // WebFrameMain should only be created for active or unloading frames.
  DCHECK(GetLifecycleState(rfh) == LifecycleState::kActive ||
         GetLifecycleState(rfh) == LifecycleState::kRunningUnloadHandlers);
}

WebFrameMain::~WebFrameMain() {
  Destroyed();
}

void WebFrameMain::Destroyed() {
  if (FromFrameTreeNodeId(frame_tree_node_id_) == this) {
    // WebFrameMain initialized as detached doesn't support FTN lookup and
    // shouldn't erase the entry.
    DCHECK(!render_frame_detached_ || render_frame_disposed_);
    GetFrameTreeNodeIdMap().erase(frame_tree_node_id_);
  }

  GetFrameTokenMap().erase(frame_token_);
  MarkRenderFrameDisposed();
  Unpin();
}

void WebFrameMain::MarkRenderFrameDisposed() {
  render_frame_detached_ = true;
  render_frame_disposed_ = true;
  TeardownMojoConnection();
}

// Should only be called when swapping frames.
void WebFrameMain::UpdateRenderFrameHost(content::RenderFrameHost* rfh) {
  GetFrameTokenMap().erase(frame_token_);

  // Ensure that RFH being swapped in doesn't already exist as its own
  // WebFrameMain instance.
  frame_token_ = rfh->GetGlobalFrameToken();
  const auto [_, inserted] = GetFrameTokenMap().emplace(frame_token_, this);
  DCHECK(inserted);

  render_frame_disposed_ = false;
  TeardownMojoConnection();
  MaybeSetupMojoConnection();
}

bool WebFrameMain::CheckRenderFrame() const {
  if (!HasRenderFrame()) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Render frame was disposed before WebFrameMain could be accessed");
    return false;
  }
  return true;
}

v8::Local<v8::Promise> WebFrameMain::ExecuteJavaScript(
    gin::Arguments* args,
    const std::u16string& code) {
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

  static_cast<content::RenderFrameHostImpl*>(render_frame_host())
      ->ExecuteJavaScriptForTests(
          code, user_gesture, true /* resolve_promises */,
          /*honor_js_content_settings=*/true, content::ISOLATED_WORLD_ID_GLOBAL,
          base::BindOnce(
              [](gin_helper::Promise<base::Value> promise,
                 blink::mojom::JavaScriptExecutionResultType type,
                 base::Value value) {
                if (type ==
                    blink::mojom::JavaScriptExecutionResultType::kSuccess) {
                  promise.Resolve(value);
                } else {
                  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
                  v8::HandleScope scope(isolate);
                  promise.Reject(gin::ConvertToV8(isolate, value));
                }
              },
              std::move(promise)));

  return handle;
}

bool WebFrameMain::Reload() {
  if (!CheckRenderFrame())
    return false;
  return render_frame_host()->Reload();
}

bool WebFrameMain::IsDestroyed() const {
  return render_frame_disposed_;
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

  GetRendererApi()->Message(internal, channel, std::move(message));
}

const mojo::Remote<mojom::ElectronRenderer>& WebFrameMain::GetRendererApi() {
  MaybeSetupMojoConnection();
  return renderer_api_;
}

void WebFrameMain::MaybeSetupMojoConnection() {
  if (render_frame_disposed_) {
    // RFH may not be set yet if called between when a new RFH is created and
    // before it's been swapped with an old RFH.
    LOG(INFO) << "Attempt to setup WebFrameMain connection while render frame "
                 "is disposed";
    return;
  }

  if (!renderer_api_) {
    pending_receiver_ = renderer_api_.BindNewPipeAndPassReceiver();
    renderer_api_.set_disconnect_handler(base::BindOnce(
        &WebFrameMain::OnRendererConnectionError, weak_factory_.GetWeakPtr()));
  }

  content::RenderFrameHost* rfh = render_frame_host();
  DCHECK(rfh);

  // Wait for RenderFrame to be created in renderer before accessing remote.
  if (pending_receiver_ && rfh && rfh->IsRenderFrameLive()) {
    rfh->GetRemoteInterfaces()->GetInterface(std::move(pending_receiver_));
  }
}

void WebFrameMain::TeardownMojoConnection() {
  renderer_api_.reset();
  pending_receiver_.reset();
}

void WebFrameMain::OnRendererConnectionError() {
  TeardownMojoConnection();
}

[[nodiscard]] bool WebFrameMain::HasRenderFrame() const {
  if (render_frame_disposed_)
    return false;

  // If RFH is a nullptr, this instance of WebFrameMain is dangling and wasn't
  // properly deleted.
  CHECK(render_frame_host());

  return true;
}

void WebFrameMain::PostMessage(v8::Isolate* isolate,
                               const std::string& channel,
                               v8::Local<v8::Value> message_value,
                               std::optional<v8::Local<v8::Value>> transfer) {
  blink::TransferableMessage transferable_message;
  if (!electron::SerializeV8Value(isolate, message_value,
                                  &transferable_message)) {
    // SerializeV8Value sets an exception.
    return;
  }

  std::vector<gin_helper::Handle<MessagePort>> wrapped_ports;
  if (transfer && !transfer.value()->IsUndefined()) {
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

bool WebFrameMain::Detached() const {
  return render_frame_detached_;
}

content::FrameTreeNodeId WebFrameMain::FrameTreeNodeID() const {
  return frame_tree_node_id_;
}

std::string WebFrameMain::Name() const {
  if (!CheckRenderFrame())
    return {};
  return render_frame_host()->GetFrameName();
}

std::string WebFrameMain::FrameToken() const {
  if (!CheckRenderFrame())
    return "";
  const blink::LocalFrameToken& frame_token =
      render_frame_host()->GetFrameToken();
  return frame_token.ToString();
}

base::ProcessId WebFrameMain::OSProcessID() const {
  if (!CheckRenderFrame())
    return -1;
  base::ProcessHandle process_handle =
      render_frame_host()->GetProcess()->GetProcess().Handle();
  return base::GetProcId(process_handle);
}

int32_t WebFrameMain::ProcessID() const {
  if (!CheckRenderFrame())
    return -1;
  return render_frame_host()->GetProcess()->GetID().GetUnsafeValue();
}

int WebFrameMain::RoutingID() const {
  if (!CheckRenderFrame())
    return -1;
  return render_frame_host()->GetRoutingID();
}

GURL WebFrameMain::URL() const {
  if (!CheckRenderFrame())
    return {};
  return render_frame_host()->GetLastCommittedURL();
}

std::string WebFrameMain::Origin() const {
  if (!CheckRenderFrame())
    return {};
  return render_frame_host()->GetLastCommittedOrigin().Serialize();
}

blink::mojom::PageVisibilityState WebFrameMain::VisibilityState() const {
  if (!CheckRenderFrame())
    return blink::mojom::PageVisibilityState::kHidden;
  return render_frame_host()->GetVisibilityState();
}

content::RenderFrameHost* WebFrameMain::Top() const {
  if (!CheckRenderFrame())
    return nullptr;
  return render_frame_host()->GetMainFrame();
}

content::RenderFrameHost* WebFrameMain::Parent() const {
  if (!CheckRenderFrame())
    return nullptr;
  return render_frame_host()->GetParent();
}

std::vector<content::RenderFrameHost*> WebFrameMain::Frames() const {
  std::vector<content::RenderFrameHost*> frame_hosts;
  if (!CheckRenderFrame())
    return frame_hosts;

  render_frame_host()->ForEachRenderFrameHost(
      [&frame_hosts, this](content::RenderFrameHost* rfh) {
        if (rfh && rfh->GetParent() == render_frame_host())
          frame_hosts.push_back(rfh);
      });

  return frame_hosts;
}

std::vector<content::RenderFrameHost*> WebFrameMain::FramesInSubtree() const {
  std::vector<content::RenderFrameHost*> frame_hosts;
  if (!CheckRenderFrame())
    return frame_hosts;

  render_frame_host()->ForEachRenderFrameHost(
      [&frame_hosts](content::RenderFrameHost* rfh) {
        frame_hosts.push_back(rfh);
      });

  return frame_hosts;
}

std::string_view WebFrameMain::LifecycleStateForTesting() const {
  if (!HasRenderFrame())
    return {};
  if (const char* str =
          content::RenderFrameHostImpl::LifecycleStateImplToString(
              GetLifecycleState(render_frame_host())))
    return str;
  return {};
}

v8::Local<v8::Promise> WebFrameMain::CollectDocumentJSCallStack(
    gin::Arguments* args) {
  gin_helper::Promise<base::Value> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!HasRenderFrame()) {
    promise.RejectWithErrorMessage(
        "Render frame was disposed before WebFrameMain could be accessed");
    return handle;
  }

  if (!base::FeatureList::IsEnabled(
          blink::features::kDocumentPolicyIncludeJSCallStacksInCrashReports)) {
    promise.RejectWithErrorMessage(
        "DocumentPolicyIncludeJSCallStacksInCrashReports is not enabled");
    return handle;
  }

  content::RenderProcessHostImpl* rph_impl =
      static_cast<content::RenderProcessHostImpl*>(
          render_frame_host()->GetProcess());

  rph_impl->GetJavaScriptCallStackGeneratorInterface()
      ->CollectJavaScriptCallStack(
          base::BindOnce(&WebFrameMain::CollectedJavaScriptCallStack,
                         weak_factory_.GetWeakPtr(), std::move(promise)));

  return handle;
}

void WebFrameMain::CollectedJavaScriptCallStack(
    gin_helper::Promise<base::Value> promise,
    const std::string& untrusted_javascript_call_stack,
    const std::optional<blink::LocalFrameToken>& remote_frame_token) {
  if (!HasRenderFrame()) {
    promise.RejectWithErrorMessage(
        "Render frame was disposed before call stack was received");
    return;
  }

  const blink::LocalFrameToken& frame_token =
      render_frame_host()->GetFrameToken();
  if (remote_frame_token == frame_token) {
    promise.Resolve(base::Value(untrusted_javascript_call_stack));
  } else if (!remote_frame_token) {
    // Failed to collect call stack. See logic in:
    // third_party/blink/renderer/controller/javascript_call_stack_collector.cc
    promise.Resolve(base::Value());
  } else {
    // Requests for call stacks can be initiated on an old RenderProcessHost
    // then be received after a frame swap.
    LOG(ERROR) << "Received call stack from old RPH";
    promise.Resolve(base::Value());
  }
}

void WebFrameMain::DOMContentLoaded() {
  Emit("dom-ready");
}

// static
gin_helper::Handle<WebFrameMain> WebFrameMain::New(v8::Isolate* isolate) {
  return {};
}

// static
gin_helper::Handle<WebFrameMain> WebFrameMain::From(
    v8::Isolate* isolate,
    content::RenderFrameHost* rfh) {
  if (!rfh)
    return {};

  WebFrameMain* web_frame;
  switch (GetLifecycleState(rfh)) {
    case LifecycleState::kSpeculative:
    case LifecycleState::kPendingCommit:
      // RFH is in the process of being swapped. Need to lookup by FTN to avoid
      // creating dangling WebFrameMain.
      web_frame = FromFrameTreeNodeId(rfh->GetFrameTreeNodeId());
      break;
    case LifecycleState::kPrerendering:
    case LifecycleState::kActive:
    case LifecycleState::kInBackForwardCache:
      // RFH is already assigned to the FrameTreeNode and can safely be looked
      // up directly.
      web_frame = FromRenderFrameHost(rfh);
      break;
    case LifecycleState::kRunningUnloadHandlers:
      // Event/IPC emitted for a frame running unload handlers. Return the exact
      // RFH so the security origin will be accurate.
      web_frame = FromRenderFrameHost(rfh);
      break;
    case LifecycleState::kReadyToBeDeleted:
      // RFH is gone
      return {};
  }

  if (web_frame)
    return gin_helper::CreateHandle(isolate, web_frame);

  auto handle = gin_helper::CreateHandle(isolate, new WebFrameMain(rfh));

  // Prevent garbage collection of frame until it has been deleted internally.
  handle->Pin(isolate);

  return handle;
}

// static
void WebFrameMain::FillObjectTemplate(v8::Isolate* isolate,
                                      v8::Local<v8::ObjectTemplate> templ) {
  gin_helper::ObjectTemplateBuilder(isolate, templ)
      .SetMethod("executeJavaScript", &WebFrameMain::ExecuteJavaScript)
      .SetMethod("collectJavaScriptCallStack",
                 &WebFrameMain::CollectDocumentJSCallStack)
      .SetMethod("reload", &WebFrameMain::Reload)
      .SetMethod("isDestroyed", &WebFrameMain::IsDestroyed)
      .SetMethod("_send", &WebFrameMain::Send)
      .SetMethod("_postMessage", &WebFrameMain::PostMessage)
      .SetProperty("detached", &WebFrameMain::Detached)
      .SetProperty("frameTreeNodeId", &WebFrameMain::FrameTreeNodeID)
      .SetProperty("name", &WebFrameMain::Name)
      .SetProperty("frameToken", &WebFrameMain::FrameToken)
      .SetProperty("osProcessId", &WebFrameMain::OSProcessID)
      .SetProperty("processId", &WebFrameMain::ProcessID)
      .SetProperty("routingId", &WebFrameMain::RoutingID)
      .SetProperty("url", &WebFrameMain::URL)
      .SetProperty("origin", &WebFrameMain::Origin)
      .SetProperty("visibilityState", &WebFrameMain::VisibilityState)
      .SetProperty("top", &WebFrameMain::Top)
      .SetProperty("parent", &WebFrameMain::Parent)
      .SetProperty("frames", &WebFrameMain::Frames)
      .SetProperty("framesInSubtree", &WebFrameMain::FramesInSubtree)
      .SetProperty("_lifecycleStateForTesting",
                   &WebFrameMain::LifecycleStateForTesting)
      .Build();
}

const char* WebFrameMain::GetTypeName() {
  return GetClassName();
}

}  // namespace electron::api

namespace {

using electron::api::WebFrameMain;

v8::Local<v8::Value> FromID(gin_helper::ErrorThrower thrower,
                            int render_process_id,
                            int render_frame_id) {
  if (!electron::Browser::Get()->is_ready()) {
    thrower.ThrowError("WebFrameMain is available only after app ready");
    return v8::Null(thrower.isolate());
  }

  auto* rfh =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  if (!rfh)
    return v8::Undefined(thrower.isolate());

  return WebFrameMain::From(thrower.isolate(), rfh).ToV8();
}

v8::Local<v8::Value> FromFrameToken(gin_helper::ErrorThrower thrower,
                                    int render_process_id,
                                    std::string render_frame_token) {
  if (!electron::Browser::Get()->is_ready()) {
    thrower.ThrowError("WebFrameMain is available only after app ready");
    return v8::Null(thrower.isolate());
  }

  auto token = base::Token::FromString(render_frame_token);
  if (!token)
    return v8::Null(thrower.isolate());
  auto unguessable_token =
      base::UnguessableToken::Deserialize(token->high(), token->low());
  if (!unguessable_token)
    return v8::Null(thrower.isolate());
  auto frame_token = blink::LocalFrameToken(unguessable_token.value());

  auto* rfh = content::RenderFrameHost::FromFrameToken(
      content::GlobalRenderFrameHostToken(render_process_id, frame_token));
  if (!rfh)
    return v8::Null(thrower.isolate());

  return WebFrameMain::From(thrower.isolate(), rfh).ToV8();
}

v8::Local<v8::Value> FromIdIfExists(gin_helper::ErrorThrower thrower,
                                    int render_process_id,
                                    int render_frame_id) {
  if (!electron::Browser::Get()->is_ready()) {
    thrower.ThrowError("WebFrameMain is available only after app ready");
    return v8::Null(thrower.isolate());
  }
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  WebFrameMain* web_frame = WebFrameMain::FromRenderFrameHost(rfh);
  if (!web_frame)
    return v8::Null(thrower.isolate());
  return gin_helper::CreateHandle(thrower.isolate(), web_frame).ToV8();
}

v8::Local<v8::Value> FromFtnIdIfExists(gin_helper::ErrorThrower thrower,
                                       int frame_tree_node_id) {
  if (!electron::Browser::Get()->is_ready()) {
    thrower.ThrowError("WebFrameMain is available only after app ready");
    return v8::Null(thrower.isolate());
  }
  WebFrameMain* web_frame = WebFrameMain::FromFrameTreeNodeId(
      content::FrameTreeNodeId(frame_tree_node_id));
  if (!web_frame)
    return v8::Null(thrower.isolate());
  return gin_helper::CreateHandle(thrower.isolate(), web_frame).ToV8();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("WebFrameMain", WebFrameMain::GetConstructor(isolate, context));
  dict.SetMethod("fromId", &FromID);
  dict.SetMethod("fromFrameToken", &FromFrameToken);
  dict.SetMethod("_fromIdIfExists", &FromIdIfExists);
  dict.SetMethod("_fromFtnIdIfExists", &FromFtnIdIfExists);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_web_frame_main, Initialize)
