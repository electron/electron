// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/renderer/electron_api_service_impl.h"

#include <string_view>
#include <utility>
#include <vector>

#include "base/trace_event/trace_event.h"
#include "gin/data_object_builder.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/heap_snapshot.h"
#include "shell/common/options_switches.h"
#include "shell/common/thread_restrictions.h"
#include "shell/common/v8_util.h"
#include "shell/renderer/electron_ipc_native.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "shell/renderer/renderer_client_base.h"
#include "third_party/blink/public/mojom/frame/user_activation_notification_type.mojom-shared.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_message_port_converter.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/layout/hit_test_location.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/point_f.h"
#include "url/gurl.h"
#include "url/url_constants.h"
#include "v8/include/v8-context.h"

namespace electron {

namespace {

bool IsSaveableImageNode(blink::Node* node) {
  return node && (blink::IsA<blink::HTMLCanvasElement>(node) ||
                  !blink::HitTestResult::AbsoluteImageURL(node).IsEmpty());
}

blink::Node* GetSaveableImageNode(const blink::HitTestResult& result) {
  if (blink::Node* node = result.InnerNodeOrImageMapImage();
      IsSaveableImageNode(node)) {
    return node;
  }

  for (const auto& raw_node : result.ListBasedTestResult()) {
    blink::Node* node = raw_node.Get();
    if (IsSaveableImageNode(node))
      return node;
  }

  return nullptr;
}

mojom::ImageSaveInfoPtr GetImageSaveInfo(content::RenderFrame* render_frame,
                                         int x,
                                         int y) {
  auto* web_frame = render_frame->GetWebFrame();
  if (!web_frame)
    return nullptr;

  blink::WebFrameWidget* widget = web_frame->FrameWidget();
  if (!widget)
    return nullptr;

  auto* frame = static_cast<blink::WebLocalFrameImpl*>(web_frame)->GetFrame();
  if (!frame)
    return nullptr;

  const gfx::Point viewport_point =
      gfx::ToRoundedPoint(widget->DIPsToBlinkSpace(gfx::PointF(x, y)));
  const gfx::Point root_frame_point(
      frame->GetPage()->GetVisualViewport().ViewportToRootFrame(
          viewport_point));
  blink::HitTestLocation location(
      frame->View()->ConvertFromRootFrame(root_frame_point));
  const auto hit_test_type = blink::HitTestRequest::kReadOnly |
                             blink::HitTestRequest::kActive |
                             blink::HitTestRequest::kPenetratingList |
                             blink::HitTestRequest::kListBased;
  blink::HitTestResult result =
      frame->GetEventHandler().HitTestResultAtLocation(location, hit_test_type);
  result.SetToShadowHostIfInUAShadowRoot();

  blink::Node* image_node = GetSaveableImageNode(result);
  if (!image_node)
    return nullptr;

  auto info = mojom::ImageSaveInfo::New();

  if (blink::IsA<blink::HTMLCanvasElement>(image_node)) {
    info->use_save_image_at = true;
  } else if (!blink::HitTestResult::AbsoluteImageURL(image_node).IsEmpty()) {
    info->src_url = GURL(blink::HitTestResult::AbsoluteImageURL(image_node));
    info->use_save_image_at = info->src_url.SchemeIs(url::kDataScheme);
  } else {
    return nullptr;
  }

  blink::Document& document = image_node->GetDocument();
  info->frame_url = GURL(document.Url());
  info->referrer_policy = document.GetReferrerPolicy();
  info->is_image_media_plugin_document =
      document.IsImageDocument() || document.IsMediaDocument() ||
      document.IsPluginDocument() || info->frame_url == info->src_url;

  return info;
}

}  // namespace

ElectronApiServiceImpl::~ElectronApiServiceImpl() = default;

ElectronApiServiceImpl::ElectronApiServiceImpl(
    content::RenderFrame* render_frame,
    RendererClientBase* renderer_client)
    : content::RenderFrameObserver(render_frame),
      renderer_client_(renderer_client) {
  registry_.AddInterface<mojom::ElectronRenderer>(base::BindRepeating(
      &ElectronApiServiceImpl::BindTo, base::Unretained(this)));
}

void ElectronApiServiceImpl::BindTo(
    mojo::PendingReceiver<mojom::ElectronRenderer> receiver) {
  if (document_created_) {
    if (receiver_.is_bound())
      receiver_.reset();

    receiver_.Bind(std::move(receiver));
    receiver_.set_disconnect_handler(base::BindOnce(
        &ElectronApiServiceImpl::OnConnectionError, GetWeakPtr()));
  } else {
    pending_receiver_ = std::move(receiver);
  }
}

void ElectronApiServiceImpl::OnInterfaceRequestForFrame(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  registry_.TryBindInterface(interface_name, interface_pipe);
}

void ElectronApiServiceImpl::DidCreateDocumentElement() {
  document_created_ = true;

  if (pending_receiver_) {
    if (receiver_.is_bound())
      receiver_.reset();

    receiver_.Bind(std::move(pending_receiver_));
    receiver_.set_disconnect_handler(base::BindOnce(
        &ElectronApiServiceImpl::OnConnectionError, GetWeakPtr()));
  }
}

void ElectronApiServiceImpl::OnDestruct() {
  delete this;
}

void ElectronApiServiceImpl::OnConnectionError() {
  if (receiver_.is_bound())
    receiver_.reset();
}

void ElectronApiServiceImpl::Message(bool internal,
                                     const std::string& channel,
                                     blink::CloneableMessage arguments) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  if (!frame)
    return;

  v8::Isolate* isolate = frame->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = renderer_client_->GetContext(frame, isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Value> args = gin::ConvertToV8(isolate, arguments);

  ipc_native::EmitIPCEvent(isolate, context, internal, channel, {}, args);
}

void ElectronApiServiceImpl::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  if (!frame)
    return;

  v8::Isolate* isolate = frame->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = renderer_client_->GetContext(frame, isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Value> message_value = DeserializeV8Value(isolate, message);

  std::vector<v8::Local<v8::Value>> ports;
  for (auto& port : message.ports) {
    ports.emplace_back(
        blink::WebMessagePortConverter::EntangleAndInjectMessagePortChannel(
            isolate, context, std::move(port)));
  }

  std::vector<v8::Local<v8::Value>> args = {message_value};

  ipc_native::EmitIPCEvent(isolate, context, false, channel, ports,
                           gin::ConvertToV8(isolate, args));
}

void ElectronApiServiceImpl::GetImageSaveInfoAt(
    int32_t x,
    int32_t y,
    GetImageSaveInfoAtCallback callback) {
  std::move(callback).Run(GetImageSaveInfo(render_frame(), x, y));
}

void ElectronApiServiceImpl::TakeHeapSnapshot(
    mojo::ScopedHandle file,
    TakeHeapSnapshotCallback callback) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  if (!frame)
    return;

  ScopedAllowBlockingForElectron allow_blocking;

  base::ScopedPlatformFile platform_file;
  if (mojo::UnwrapPlatformFile(std::move(file), &platform_file) !=
      MOJO_RESULT_OK) {
    LOG(ERROR) << "Unable to get the file handle from mojo.";
    std::move(callback).Run(false);
    return;
  }
  base::File base_file(std::move(platform_file));

  v8::Isolate* isolate = frame->GetAgentGroupScheduler()->Isolate();
  bool success = electron::TakeHeapSnapshot(isolate, &base_file);

  std::move(callback).Run(success);
}

}  // namespace electron
