// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/osr/osr_web_contents_view.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "content/browser/web_contents/web_contents_impl.h"  // nogncheck
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/drop_data.h"
#include "shell/browser/native_window.h"
#include "shell/browser/osr/osr_drag_delegate.h"
#include "third_party/blink/public/common/input/web_keyboard_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "ui/display/screen.h"
#include "ui/display/screen_info.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace electron {

OffScreenWebContentsView::DragState::DragState() = default;
OffScreenWebContentsView::DragState::DragState(DragState&&) = default;
OffScreenWebContentsView::DragState::~DragState() = default;

OffScreenWebContentsView::OffScreenWebContentsView(
    bool transparent,
    bool offscreen_use_shared_texture,
    const std::string& offscreen_shared_texture_pixel_format,
    float offscreen_device_scale_factor,
    const OnPaintCallback& callback)
    : transparent_(transparent),
      offscreen_use_shared_texture_(offscreen_use_shared_texture),
      offscreen_shared_texture_pixel_format_(
          offscreen_shared_texture_pixel_format),
      offscreen_device_scale_factor_(offscreen_device_scale_factor),
      callback_(callback) {
#if BUILDFLAG(IS_MAC)
  PlatformCreate();
#endif
}

OffScreenWebContentsView::~OffScreenWebContentsView() {
  if (native_window_)
    native_window_->RemoveObserver(this);

#if BUILDFLAG(IS_MAC)
  PlatformDestroy();
#endif
}

void OffScreenWebContentsView::SetWebContents(
    content::WebContents* web_contents) {
  web_contents_ = web_contents;

  if (auto* view = GetView())
    view->InstallTransparency();
}

void OffScreenWebContentsView::SetCallback(const OnPaintCallback& callback) {
  callback_ = callback;
}

void OffScreenWebContentsView::SetDragDelegate(
    OffScreenDragDelegate* delegate) {
  drag_delegate_ = delegate;
  if (!drag_delegate_)
    CancelDrag();
}

void OffScreenWebContentsView::SetNativeWindow(NativeWindow* window) {
  if (native_window_)
    native_window_->RemoveObserver(this);

  native_window_ = window;

  if (native_window_)
    native_window_->AddObserver(this);

  OnWindowResize();
}

void OffScreenWebContentsView::OnWindowResize() {
  // In offscreen mode call RenderWidgetHostView's SetSize explicitly
  if (auto* view = GetView())
    view->SetSize(GetSize());
}

void OffScreenWebContentsView::OnWindowClosed() {
  if (native_window_) {
    native_window_->RemoveObserver(this);
    native_window_ = nullptr;
  }
}

gfx::Size OffScreenWebContentsView::GetSize() const {
  return native_window_ ? native_window_->GetSize() : gfx::Size();
}

#if !BUILDFLAG(IS_MAC)
gfx::NativeView OffScreenWebContentsView::GetNativeView() const {
  if (!native_window_)
    return {};
  return native_window_->GetNativeView();
}

gfx::NativeView OffScreenWebContentsView::GetContentNativeView() const {
  if (!native_window_)
    return {};
  return native_window_->GetNativeView();
}

gfx::NativeWindow OffScreenWebContentsView::GetTopLevelNativeWindow() const {
  if (!native_window_)
    return {};
  return native_window_->GetNativeWindow();
}
#endif

gfx::Rect OffScreenWebContentsView::GetContainerBounds() const {
  return GetViewBounds();
}

content::DropData* OffScreenWebContentsView::GetDropData() const {
  return drag_ ? drag_->drop_data.get() : nullptr;
}

gfx::Rect OffScreenWebContentsView::GetViewBounds() const {
  if (auto* view = GetView())
    return view->GetViewBounds();
  return {};
}

content::RenderWidgetHostViewBase*
OffScreenWebContentsView::CreateViewForWidget(
    content::RenderWidgetHost* render_widget_host) {
  if (auto* rwhv = render_widget_host->GetView())
    return static_cast<content::RenderWidgetHostViewBase*>(rwhv);

  return new OffScreenRenderWidgetHostView(
      transparent_, offscreen_use_shared_texture_,
      offscreen_shared_texture_pixel_format_, offscreen_device_scale_factor_,
      painting_, GetFrameRate(), callback_, render_widget_host, nullptr,
      GetSize());
}

content::RenderWidgetHostViewBase*
OffScreenWebContentsView::CreateViewForChildWidget(
    content::RenderWidgetHost* render_widget_host) {
  auto* web_contents_impl =
      static_cast<content::WebContentsImpl*>(web_contents_);

  OffScreenRenderWidgetHostView* embedder_host_view = nullptr;
  if (web_contents_impl->GetOuterWebContents()) {
    embedder_host_view = static_cast<OffScreenRenderWidgetHostView*>(
        web_contents_impl->GetOuterWebContents()->GetRenderWidgetHostView());
  } else {
    embedder_host_view = static_cast<OffScreenRenderWidgetHostView*>(
        web_contents_impl->GetRenderWidgetHostView());
  }
  CHECK(embedder_host_view);

  return new OffScreenRenderWidgetHostView(
      transparent_, offscreen_use_shared_texture_,
      offscreen_shared_texture_pixel_format_, offscreen_device_scale_factor_,
      painting_, embedder_host_view->frame_rate(), callback_,
      render_widget_host, embedder_host_view, GetSize());
}

void OffScreenWebContentsView::RenderViewReady() {
  if (auto* view = GetView())
    view->InstallTransparency();
}

#if BUILDFLAG(IS_MAC)
bool OffScreenWebContentsView::CloseTabAfterEventTrackingIfNeeded() {
  return false;
}
#endif  // BUILDFLAG(IS_MAC)

void OffScreenWebContentsView::StartDragging(
    content::RenderFrameHost& source_rfh,
    const content::DropData& drop_data,
    blink::DragOperationsMask allowed_ops,
    const gfx::ImageSkia& image,
    const gfx::Vector2d& cursor_offset,
    const gfx::Rect& drag_obj_rect,
    const blink::mojom::DragEventSourceInfo& event_info) {
  auto* source_rwh = static_cast<content::RenderWidgetHostImpl*>(
      source_rfh.GetRenderWidgetHost());
  // Disallow re-entrant drags, matching WebContentsViewAura.
  if (drag_)
    return;
  if (!web_contents_ || !drag_delegate_) {
    if (web_contents_) {
      static_cast<content::WebContentsImpl*>(web_contents_)
          ->SystemDragEnded(source_rwh);
    }
    return;
  }

  drag_security_info_.OnDragInitiated(source_rwh, drop_data);
  drag_.emplace();
  drag_->drop_data = std::make_unique<content::DropData>(drop_data);
  // The data never leaves the browser, so mark it the way a round trip through
  // the OS would.
  drag_->drop_data->did_originate_from_renderer = true;
  drag_->drop_data->file_contents.clear();
  drag_->allowed_ops = allowed_ops;
  drag_->source_rwh = source_rwh->GetWeakPtr();

  drag_delegate_->OnOffScreenDragStart(image, cursor_offset, allowed_ops);
}

bool OffScreenWebContentsView::HandleDragMouseEvent(
    const blink::WebMouseEvent& event) {
  if (!drag_)
    return false;
  // The root widget sits at the view origin, so widget coordinates are client
  // coordinates; screen coordinates are whatever the embedder supplied.
  drag_->last_client_pt = event.PositionInWidget();
  drag_->last_screen_pt = event.PositionInScreen();
  switch (event.GetType()) {
    case blink::WebInputEvent::Type::kMouseMove:
    case blink::WebInputEvent::Type::kMouseEnter:
      DragTargetUpdate(event);
      break;
    case blink::WebInputEvent::Type::kMouseLeave:
      DragTargetLeave();
      SetDragOperation(ui::mojom::DragOperation::kNone);
      break;
    case blink::WebInputEvent::Type::kMouseUp: {
      auto* target = drag_->target_rwh.get();
      auto operation = drag_->operation;
      if (target && operation != ui::mojom::DragOperation::kNone) {
        target->DragTargetDrop(*drag_->drop_data, drag_->last_client_pt,
                               drag_->last_screen_pt, event.GetModifiers(),
                               base::DoNothing());
        drag_->target_rwh = nullptr;
      } else {
        DragTargetLeave();
        operation = ui::mojom::DragOperation::kNone;
      }
      EndDrag(operation, /*cancelled=*/false);
      break;
    }
    default:
      // Swallow other mouse input while the drag has the pointer.
      break;
  }
  return true;
}

bool OffScreenWebContentsView::HandleDragKeyEvent(
    const blink::WebKeyboardEvent& event) {
  if (!drag_)
    return false;
  if (event.windows_key_code == ui::VKEY_ESCAPE) {
    if (event.GetType() == blink::WebInputEvent::Type::kRawKeyDown ||
        event.GetType() == blink::WebInputEvent::Type::kKeyDown) {
      CancelDrag();
    }
    return true;
  }
  return false;
}

void OffScreenWebContentsView::CancelDrag() {
  if (!drag_)
    return;
  DragTargetLeave();
  EndDrag(ui::mojom::DragOperation::kNone, /*cancelled=*/true);
}

content::RenderWidgetHostImpl* OffScreenWebContentsView::GetDragTargetWidget()
    const {
  if (!web_contents_ || web_contents_->IsBeingDestroyed())
    return nullptr;
  auto* target = content::RenderWidgetHostImpl::From(
      web_contents_->GetRenderViewHost()->GetWidget());
  if (!target || !target->GetView() ||
      !drag_security_info_.IsValidDragTarget(target)) {
    return nullptr;
  }
  return target;
}

void OffScreenWebContentsView::DragTargetUpdate(
    const blink::WebMouseEvent& event) {
  const gfx::PointF client_pt = drag_->last_client_pt;
  const gfx::PointF screen_pt = drag_->last_screen_pt;

  auto* target = GetDragTargetWidget();
  if (drag_->target_rwh && target != drag_->target_rwh.get()) {
    DragTargetLeave();
    SetDragOperation(ui::mojom::DragOperation::kNone);
    // The delegate may have ended the drag.
    if (!drag_)
      return;
  }
  if (!target)
    return;

  auto callback =
      base::BindOnce(&OffScreenWebContentsView::OnDragOperationNegotiated,
                     weak_factory_.GetWeakPtr());
  if (!drag_->target_rwh) {
    drag_->target_rwh = target->GetWeakPtr();
    target->FilterDropData(drag_->drop_data.get());
    // Enter only dispatches `dragenter`; follow with an over so `dragover`
    // handlers can accept the drag, as WebContentsViewAura does.
    target->DragTargetDragEnter(*drag_->drop_data, client_pt, screen_pt,
                                drag_->allowed_ops, event.GetModifiers(),
                                base::DoNothing());
  }
  target->DragTargetDragOver(client_pt, screen_pt, drag_->allowed_ops,
                             event.GetModifiers(), std::move(callback));
}

void OffScreenWebContentsView::DragTargetLeave() {
  if (!drag_ || !drag_->target_rwh)
    return;
  if (web_contents_ && !web_contents_->IsBeingDestroyed()) {
    drag_->target_rwh->DragTargetDragLeave(drag_->last_client_pt,
                                           drag_->last_screen_pt);
  }
  drag_->target_rwh = nullptr;
  drag_->drop_data->document_is_handling_drag = false;
}

void OffScreenWebContentsView::OnDragOperationNegotiated(
    ui::mojom::DragOperation operation,
    bool document_is_handling_drag) {
  // Ignore late replies for a target we already left.
  if (!drag_ || !drag_->target_rwh)
    return;
  drag_->drop_data->operation = operation;
  drag_->drop_data->document_is_handling_drag = document_is_handling_drag;
  SetDragOperation(operation);
}

void OffScreenWebContentsView::SetDragOperation(
    ui::mojom::DragOperation operation) {
  if (!drag_ || drag_->operation == operation)
    return;
  drag_->operation = operation;
  // Last, since the delegate may re-enter and end the drag.
  if (drag_delegate_)
    drag_delegate_->OnOffScreenDragUpdate(operation);
}

void OffScreenWebContentsView::EndDrag(ui::mojom::DragOperation operation,
                                       bool cancelled) {
  // Take the state first; the delegate may re-enter via JS.
  DragState drag = std::move(*drag_);
  drag_.reset();
  drag_security_info_.OnDragEnded();

  if (web_contents_ && !web_contents_->IsBeingDestroyed()) {
    auto* impl = static_cast<content::WebContentsImpl*>(web_contents_);
    impl->DragSourceEndedAt(drag.last_client_pt.x(), drag.last_client_pt.y(),
                            drag.last_screen_pt.x(), drag.last_screen_pt.y(),
                            operation, drag.source_rwh.get());
    impl->SystemDragEnded(drag.source_rwh.get());
  }

  if (drag_delegate_)
    drag_delegate_->OnOffScreenDragEnd(operation, cancelled);
}

void OffScreenWebContentsView::SetPainting(bool painting) {
  painting_ = painting;
  if (auto* view = GetView())
    view->SetPainting(painting);
}

bool OffScreenWebContentsView::IsPainting() const {
  if (auto* view = GetView())
    return view->is_painting();
  return painting_;
}

void OffScreenWebContentsView::SetFrameRate(int frame_rate) {
  frame_rate_ = frame_rate;
  if (auto* view = GetView())
    view->SetFrameRate(frame_rate);
}

int OffScreenWebContentsView::GetFrameRate() const {
  if (auto* view = GetView())
    return view->frame_rate();
  return frame_rate_;
}

OffScreenRenderWidgetHostView* OffScreenWebContentsView::GetView() const {
  if (web_contents_) {
    return static_cast<OffScreenRenderWidgetHostView*>(
        web_contents_->GetRenderViewHost()->GetWidget()->GetView());
  }
  return nullptr;
}

content::BackForwardTransitionAnimationManager*
OffScreenWebContentsView::GetBackForwardTransitionAnimationManager() {
  return nullptr;
}

}  // namespace electron
