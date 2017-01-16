// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_web_contents_view.h"

#include "third_party/WebKit/public/platform/WebScreenInfo.h"

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
}

#if !defined(OS_MACOSX)
gfx::NativeView OffScreenWebContentsView::GetNativeView() const {
  return gfx::NativeView();
}

gfx::NativeView OffScreenWebContentsView::GetContentNativeView() const {
  return gfx::NativeView();
}

gfx::NativeWindow OffScreenWebContentsView::GetTopLevelNativeWindow() const {
  return gfx::NativeWindow();
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
  return view_ ? view_->GetViewBounds() : gfx::Rect();
}

void OffScreenWebContentsView::CreateView(const gfx::Size& initial_size,
                                          gfx::NativeView context) {
}

content::RenderWidgetHostViewBase*
  OffScreenWebContentsView::CreateViewForWidget(
    content::RenderWidgetHost* render_widget_host, bool is_guest_view_hack) {
  auto relay = NativeWindowRelay::FromWebContents(web_contents_);
  view_ = new OffScreenRenderWidgetHostView(
      transparent_, callback_, render_widget_host, relay->window.get());
  return view_;
}

content::RenderWidgetHostViewBase*
  OffScreenWebContentsView::CreateViewForPopupWidget(
    content::RenderWidgetHost* render_widget_host) {
  auto relay = NativeWindowRelay::FromWebContents(web_contents_);
  view_ = new OffScreenRenderWidgetHostView(
      transparent_, callback_, render_widget_host, relay->window.get());
  return view_;
}

void OffScreenWebContentsView::SetPageTitle(const base::string16& title) {
}

void OffScreenWebContentsView::RenderViewCreated(
    content::RenderViewHost* host) {
  if (view_)
    view_->InstallTransparency();
}

void OffScreenWebContentsView::RenderViewSwappedIn(
    content::RenderViewHost* host) {
}

void OffScreenWebContentsView::SetOverscrollControllerEnabled(bool enabled) {
}

void OffScreenWebContentsView::GetScreenInfo(
    blink::WebScreenInfo* web_screen_info) const {
  web_screen_info->rect = gfx::Rect(view_->size());
  web_screen_info->availableRect = gfx::Rect(view_->size());
  web_screen_info->depth = 24;
  web_screen_info->depthPerComponent = 8;
  web_screen_info->deviceScaleFactor = view_->scale_factor();
  web_screen_info->orientationAngle = 0;
  web_screen_info->orientationType =
      blink::WebScreenOrientationLandscapePrimary;
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
    const content::DragEventSourceInfo& event_info) {
  if (web_contents_)
    web_contents_->SystemDragEnded();
}

void OffScreenWebContentsView::UpdateDragCursor(
    blink::WebDragOperation operation) {
}

}  // namespace atom
