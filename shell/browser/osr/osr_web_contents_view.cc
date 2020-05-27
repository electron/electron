// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/osr/osr_web_contents_view.h"

#include "content/browser/web_contents/web_contents_impl.h"  // nogncheck
#include "content/public/browser/render_view_host.h"
#include "third_party/blink/public/platform/web_screen_info.h"
#include "ui/display/screen.h"

namespace electron {

OffScreenWebContentsView::OffScreenWebContentsView(
    bool transparent,
    const OnPaintCallback& callback)
    : native_window_(nullptr), transparent_(transparent), callback_(callback) {
#if defined(OS_MACOSX)
  PlatformCreate();
#endif
}

OffScreenWebContentsView::~OffScreenWebContentsView() {
  if (native_window_)
    native_window_->RemoveObserver(this);

#if defined(OS_MACOSX)
  PlatformDestroy();
#endif
}

void OffScreenWebContentsView::SetWebContents(
    content::WebContents* web_contents) {
  web_contents_ = web_contents;

  if (GetView())
    GetView()->InstallTransparency();
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
  if (GetView())
    GetView()->SetSize(GetSize());
}

void OffScreenWebContentsView::OnWindowClosed() {
  if (native_window_) {
    native_window_->RemoveObserver(this);
    native_window_ = nullptr;
  }
}

gfx::Size OffScreenWebContentsView::GetSize() {
  return native_window_ ? native_window_->GetSize() : gfx::Size();
}

#if !defined(OS_MACOSX)
gfx::NativeView OffScreenWebContentsView::GetNativeView() const {
  if (!native_window_)
    return gfx::NativeView();
  return native_window_->GetNativeView();
}

gfx::NativeView OffScreenWebContentsView::GetContentNativeView() const {
  if (!native_window_)
    return gfx::NativeView();
  return native_window_->GetNativeView();
}

gfx::NativeWindow OffScreenWebContentsView::GetTopLevelNativeWindow() const {
  if (!native_window_)
    return gfx::NativeWindow();
  return native_window_->GetNativeWindow();
}
#endif

gfx::Rect OffScreenWebContentsView::GetContainerBounds() const {
  return GetViewBounds();
}

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

void OffScreenWebContentsView::CreateView(gfx::NativeView context) {}

content::RenderWidgetHostViewBase*
OffScreenWebContentsView::CreateViewForWidget(
    content::RenderWidgetHost* render_widget_host) {
  if (render_widget_host->GetView()) {
    return static_cast<content::RenderWidgetHostViewBase*>(
        render_widget_host->GetView());
  }

  return new OffScreenRenderWidgetHostView(
      transparent_, painting_, GetFrameRate(), callback_, render_widget_host,
      nullptr, GetSize());
}

content::RenderWidgetHostViewBase*
OffScreenWebContentsView::CreateViewForChildWidget(
    content::RenderWidgetHost* render_widget_host) {
  content::WebContentsImpl* web_contents_impl =
      static_cast<content::WebContentsImpl*>(web_contents_);

  OffScreenRenderWidgetHostView* view =
      static_cast<OffScreenRenderWidgetHostView*>(
          web_contents_impl->GetOuterWebContents()
              ? web_contents_impl->GetOuterWebContents()
                    ->GetRenderWidgetHostView()
              : web_contents_impl->GetRenderWidgetHostView());

  return new OffScreenRenderWidgetHostView(transparent_, painting_,
                                           view->GetFrameRate(), callback_,
                                           render_widget_host, view, GetSize());
}

void OffScreenWebContentsView::SetPageTitle(const base::string16& title) {}

void OffScreenWebContentsView::RenderViewReady() {}

void OffScreenWebContentsView::RenderViewHostChanged(
    content::RenderViewHost* old_host,
    content::RenderViewHost* new_host) {}

void OffScreenWebContentsView::SetOverscrollControllerEnabled(bool enabled) {}

#if defined(OS_MACOSX)
bool OffScreenWebContentsView::CloseTabAfterEventTrackingIfNeeded() {
  return false;
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
    static_cast<content::WebContentsImpl*>(web_contents_)
        ->SystemDragEnded(source_rwh);
}

void OffScreenWebContentsView::UpdateDragCursor(
    blink::WebDragOperation operation) {}

void OffScreenWebContentsView::SetPainting(bool painting) {
  auto* view = GetView();
  painting_ = painting;
  if (view != nullptr) {
    view->SetPainting(painting);
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
  frame_rate_ = frame_rate;
  if (view != nullptr) {
    view->SetFrameRate(frame_rate);
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

}  // namespace electron
