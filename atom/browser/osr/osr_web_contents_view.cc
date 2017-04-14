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
    bool transparent, const OnPaintCallback& callback)
    : transparent_(transparent),
      callback_(callback),
      web_contents_(nullptr) {
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
  if (!web_contents_) return gfx::NativeView();

  auto relay = NativeWindowRelay::FromWebContents(web_contents_);
  if (!relay) return gfx::NativeView();
  return relay->window->GetNativeView();
}

gfx::NativeView OffScreenWebContentsView::GetContentNativeView() const {
  if (!web_contents_) return gfx::NativeView();

  auto relay = NativeWindowRelay::FromWebContents(web_contents_);
  if (!relay) return gfx::NativeView();
  return relay->window->GetNativeView();
}

gfx::NativeWindow OffScreenWebContentsView::GetTopLevelNativeWindow() const {
  if (!web_contents_) return gfx::NativeWindow();

  auto relay = NativeWindowRelay::FromWebContents(web_contents_);
  if (!relay) return gfx::NativeWindow();
  return relay->window->GetNativeWindow();
}
#endif

void OffScreenWebContentsView::GetContainerBounds(gfx::Rect* out) const {
  *out = GetViewBounds();
}

void OffScreenWebContentsView::SizeContents(const gfx::Size& size) {
}

void OffScreenWebContentsView::Focus() {
}

void OffScreenWebContentsView::SetInitialFocus() {
}

void OffScreenWebContentsView::StoreFocus() {
}

void OffScreenWebContentsView::RestoreFocus() {
}

content::DropData* OffScreenWebContentsView::GetDropData() const {
  return nullptr;
}

gfx::Rect OffScreenWebContentsView::GetViewBounds() const {
  return GetView() ? GetView()->GetViewBounds() : gfx::Rect();
}

void OffScreenWebContentsView::CreateView(const gfx::Size& initial_size,
                                          gfx::NativeView context) {
}

content::RenderWidgetHostViewBase*
  OffScreenWebContentsView::CreateViewForWidget(
    content::RenderWidgetHost* render_widget_host, bool is_guest_view_hack) {
  if (render_widget_host->GetView()) {
    return static_cast<content::RenderWidgetHostViewBase*>(
        render_widget_host->GetView());
  }

  auto relay = NativeWindowRelay::FromWebContents(web_contents_);
  return new OffScreenRenderWidgetHostView(
      transparent_,
      callback_,
      render_widget_host,
      nullptr,
      relay->window.get());
}

content::RenderWidgetHostViewBase*
  OffScreenWebContentsView::CreateViewForPopupWidget(
    content::RenderWidgetHost* render_widget_host) {
  auto relay = NativeWindowRelay::FromWebContents(web_contents_);

  content::WebContentsImpl *web_contents_impl =
    static_cast<content::WebContentsImpl*>(web_contents_);

  OffScreenRenderWidgetHostView *view =
    static_cast<OffScreenRenderWidgetHostView*>(
      web_contents_impl->GetOuterWebContents()
      ? web_contents_impl->GetOuterWebContents()->GetRenderWidgetHostView()
      : web_contents_impl->GetRenderWidgetHostView());

  return new OffScreenRenderWidgetHostView(
      transparent_,
      callback_,
      render_widget_host,
      view,
      relay->window.get());
}

void OffScreenWebContentsView::SetPageTitle(const base::string16& title) {
}

void OffScreenWebContentsView::RenderViewCreated(
    content::RenderViewHost* host) {
  if (GetView())
    GetView()->InstallTransparency();

#if defined(OS_MACOSX)
  host->Send(new AtomViewMsg_Offscreen(host->GetRoutingID()));
#endif
}

void OffScreenWebContentsView::RenderViewSwappedIn(
    content::RenderViewHost* host) {
}

void OffScreenWebContentsView::SetOverscrollControllerEnabled(bool enabled) {
}

void OffScreenWebContentsView::GetScreenInfo(
    content::ScreenInfo* screen_info) const {
  screen_info->depth = 24;
  screen_info->depth_per_component = 8;
  screen_info->orientation_angle = 0;
  screen_info->device_scale_factor = 1.0;
  screen_info->orientation_type =
      content::SCREEN_ORIENTATION_VALUES_LANDSCAPE_PRIMARY;

  if (GetView()) {
    screen_info->rect = gfx::Rect(GetView()->size());
    screen_info->available_rect = gfx::Rect(GetView()->size());
  } else {
    const display::Display display =
      display::Screen::GetScreen()->GetPrimaryDisplay();
    screen_info->rect = display.bounds();
    screen_info->available_rect = display.work_area();
  }
}

#if defined(OS_MACOSX)
void OffScreenWebContentsView::SetAllowOtherViews(bool allow) {
}

bool OffScreenWebContentsView::GetAllowOtherViews() const {
  return false;
}

bool OffScreenWebContentsView::IsEventTracking() const {
  return false;
}

void OffScreenWebContentsView::CloseTabAfterEventTracking() {
}
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
    blink::WebDragOperation operation) {
}

OffScreenRenderWidgetHostView* OffScreenWebContentsView::GetView() const {
  if (web_contents_) {
    return static_cast<OffScreenRenderWidgetHostView*>(
        web_contents_->GetRenderViewHost()->GetWidget()->GetView());
  }
  return nullptr;
}

}  // namespace atom
