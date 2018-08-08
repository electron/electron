// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_web_contents_view.h"

#include "atom/common/api/api_messages.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/render_view_host.h"
#include "third_party/WebKit/public/platform/WebScreenInfo.h"
#include "ui/display/screen.h"

namespace atom {

OffScreenWebContentsView::OffScreenWebContentsView(
    bool transparent,
    const OnPaintCallback& callback)
    : transparent_(transparent), callback_(callback) {
#if defined(OS_MACOSX)
  PlatformCreate();
#endif
}

OffScreenWebContentsView::~OffScreenWebContentsView() {
#if defined(OS_MACOSX)
  PlatformDestroy();
#endif
}

void OffScreenWebContentsView::SetWebContents(
    content::WebContents* web_contents) {
  web_contents_ = web_contents;

  RenderViewCreated(web_contents_->GetRenderViewHost());
}

#if !defined(OS_MACOSX)
gfx::NativeView OffScreenWebContentsView::GetNativeView() const {
  if (!web_contents_)
    return gfx::NativeView();

  auto* relay = NativeWindowRelay::FromWebContents(web_contents_);
  if (!relay)
    return gfx::NativeView();
  return relay->window->GetNativeView();
}

gfx::NativeView OffScreenWebContentsView::GetContentNativeView() const {
  if (!web_contents_)
    return gfx::NativeView();

  auto* relay = NativeWindowRelay::FromWebContents(web_contents_);
  if (!relay)
    return gfx::NativeView();
  return relay->window->GetNativeView();
}

gfx::NativeWindow OffScreenWebContentsView::GetTopLevelNativeWindow() const {
  if (!web_contents_)
    return gfx::NativeWindow();

  auto* relay = NativeWindowRelay::FromWebContents(web_contents_);
  if (!relay)
    return gfx::NativeWindow();
  return relay->window->GetNativeWindow();
}
#endif

void OffScreenWebContentsView::GetContainerBounds(gfx::Rect* out) const {
  *out = GetViewBounds();
}

void OffScreenWebContentsView::SizeContents(const gfx::Size& size) {}

void OffScreenWebContentsView::Focus() {}

void OffScreenWebContentsView::SetInitialFocus() {}

void OffScreenWebContentsView::StoreFocus() {}

void OffScreenWebContentsView::RestoreFocus() {}

void OffScreenWebContentsView::FocusThroughTabTraversal(bool reverse) {}

content::DropData* OffScreenWebContentsView::GetDropData() const {
  return nullptr;
}

gfx::Rect OffScreenWebContentsView::GetViewBounds() const {
  return GetView() ? GetView()->GetViewBounds() : gfx::Rect();
}

void OffScreenWebContentsView::CreateView(const gfx::Size& initial_size,
                                          gfx::NativeView context) {}

content::RenderWidgetHostViewBase*
OffScreenWebContentsView::CreateViewForWidget(
    content::RenderWidgetHost* render_widget_host,
    bool is_guest_view_hack) {
  if (render_widget_host->GetView()) {
    return static_cast<content::RenderWidgetHostViewBase*>(
        render_widget_host->GetView());
  }

  auto* relay = NativeWindowRelay::FromWebContents(web_contents_);
  return new OffScreenRenderWidgetHostView(
      transparent_, painting_, GetFrameRate(), callback_, render_widget_host,
      nullptr, relay->window.get());
}

content::RenderWidgetHostViewBase*
OffScreenWebContentsView::CreateViewForPopupWidget(
    content::RenderWidgetHost* render_widget_host) {
  auto* relay = NativeWindowRelay::FromWebContents(web_contents_);

  content::WebContentsImpl* web_contents_impl =
      static_cast<content::WebContentsImpl*>(web_contents_);

  OffScreenRenderWidgetHostView* view =
      static_cast<OffScreenRenderWidgetHostView*>(
          web_contents_impl->GetOuterWebContents()
              ? web_contents_impl->GetOuterWebContents()
                    ->GetRenderWidgetHostView()
              : web_contents_impl->GetRenderWidgetHostView());

  return new OffScreenRenderWidgetHostView(
      transparent_, true, view->GetFrameRate(), callback_, render_widget_host,
      view, relay->window.get());
}

void OffScreenWebContentsView::SetPageTitle(const base::string16& title) {}

void OffScreenWebContentsView::RenderViewCreated(
    content::RenderViewHost* host) {
  if (GetView())
    GetView()->InstallTransparency();

#if defined(OS_MACOSX)
  host->Send(new AtomViewMsg_Offscreen(host->GetRoutingID()));
#endif
}

void OffScreenWebContentsView::RenderViewSwappedIn(
    content::RenderViewHost* host) {}

void OffScreenWebContentsView::SetOverscrollControllerEnabled(bool enabled) {}

#if defined(OS_MACOSX)
void OffScreenWebContentsView::SetAllowOtherViews(bool allow) {}

bool OffScreenWebContentsView::GetAllowOtherViews() const {
  return false;
}

bool OffScreenWebContentsView::IsEventTracking() const {
  return false;
}

void OffScreenWebContentsView::CloseTabAfterEventTracking() {}
#endif  // defined(OS_MACOSX)

void OffScreenWebContentsView::StartDragging(
    const content::DropData& drop_data,
    blink::WebDragOperationsMask allowed_ops,
    const gfx::ImageSkia& image,
    const gfx::Vector2d& image_offset,
    const content::DragEventSourceInfo& event_info,
    content::RenderWidgetHostImpl* source_rwh) {
  if (web_contents_)
    web_contents_->SystemDragEnded(source_rwh);
}

void OffScreenWebContentsView::UpdateDragCursor(
    blink::WebDragOperation operation) {}

void OffScreenWebContentsView::SetPainting(bool painting) {
  auto* view = GetView();
  if (view != nullptr) {
    view->SetPainting(painting);
  } else {
    painting_ = painting;
  }
}

bool OffScreenWebContentsView::IsPainting() const {
  auto* view = GetView();
  if (view != nullptr) {
    return view->IsPainting();
  } else {
    return painting_;
  }
}

void OffScreenWebContentsView::SetFrameRate(int frame_rate) {
  auto* view = GetView();
  if (view != nullptr) {
    view->SetFrameRate(frame_rate);
  } else {
    frame_rate_ = frame_rate;
  }
}

int OffScreenWebContentsView::GetFrameRate() const {
  auto* view = GetView();
  if (view != nullptr) {
    return view->GetFrameRate();
  } else {
    return frame_rate_;
  }
}

OffScreenRenderWidgetHostView* OffScreenWebContentsView::GetView() const {
  if (web_contents_) {
    return static_cast<OffScreenRenderWidgetHostView*>(
        web_contents_->GetRenderViewHost()->GetWidget()->GetView());
  }
  return nullptr;
}

}  // namespace atom
