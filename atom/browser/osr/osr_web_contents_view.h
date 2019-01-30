// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_OSR_OSR_WEB_CONTENTS_VIEW_H_
#define ATOM_BROWSER_OSR_OSR_WEB_CONTENTS_VIEW_H_

#include "atom/browser/native_window.h"
#include "atom/browser/native_window_observer.h"

#include "atom/browser/osr/osr_render_widget_host_view.h"
#include "content/browser/renderer_host/render_view_host_delegate_view.h"
#include "content/browser/web_contents/web_contents_view.h"
#include "content/public/browser/web_contents.h"

#if defined(OS_MACOSX)
#ifdef __OBJC__
@class OffScreenView;
#else
class OffScreenView;
#endif
#endif

namespace atom {

class OffScreenWebContentsView : public content::WebContentsView,
                                 public content::RenderViewHostDelegateView,
                                 public NativeWindowObserver {
 public:
  OffScreenWebContentsView(bool transparent, const OnPaintCallback& callback);
  ~OffScreenWebContentsView() override;

  void SetWebContents(content::WebContents*);
  void SetNativeWindow(NativeWindow* window);

  // NativeWindowObserver:
  void OnWindowResize() override;
  void OnWindowClosed() override;

  gfx::Size GetSize();

  // content::WebContentsView:
  gfx::NativeView GetNativeView() const override;
  gfx::NativeView GetContentNativeView() const override;
  gfx::NativeWindow GetTopLevelNativeWindow() const override;
  void GetContainerBounds(gfx::Rect* out) const override;
  void SizeContents(const gfx::Size& size) override;
  void Focus() override;
  void SetInitialFocus() override;
  void StoreFocus() override;
  void RestoreFocus() override;
  void FocusThroughTabTraversal(bool reverse) override;
  content::DropData* GetDropData() const override;
  gfx::Rect GetViewBounds() const override;
  void CreateView(const gfx::Size& initial_size,
                  gfx::NativeView context) override;
  content::RenderWidgetHostViewBase* CreateViewForWidget(
      content::RenderWidgetHost* render_widget_host,
      bool is_guest_view_hack) override;
  content::RenderWidgetHostViewBase* CreateViewForChildWidget(
      content::RenderWidgetHost* render_widget_host) override;
  void SetPageTitle(const base::string16& title) override;
  void RenderViewCreated(content::RenderViewHost* host) override;
  void RenderViewReady() override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
  void SetOverscrollControllerEnabled(bool enabled) override;

#if defined(OS_MACOSX)
  bool IsEventTracking() const override;
  void CloseTabAfterEventTracking() override;
#endif

  // content::RenderViewHostDelegateView
  void StartDragging(const content::DropData& drop_data,
                     blink::WebDragOperationsMask allowed_ops,
                     const gfx::ImageSkia& image,
                     const gfx::Vector2d& image_offset,
                     const content::DragEventSourceInfo& event_info,
                     content::RenderWidgetHostImpl* source_rwh) override;
  void UpdateDragCursor(blink::WebDragOperation operation) override;

  void SetPainting(bool painting);
  bool IsPainting() const;
  void SetFrameRate(int frame_rate);
  int GetFrameRate() const;

 private:
#if defined(OS_MACOSX)
  void PlatformCreate();
  void PlatformDestroy();
#endif

  OffScreenRenderWidgetHostView* GetView() const;

  NativeWindow* native_window_;

  const bool transparent_;
  bool painting_ = true;
  int frame_rate_ = 60;
  OnPaintCallback callback_;

  // Weak refs.
  content::WebContents* web_contents_ = nullptr;

#if defined(OS_MACOSX)
  OffScreenView* offScreenView_;
#endif
};

}  // namespace atom

#endif  // ATOM_BROWSER_OSR_OSR_WEB_CONTENTS_VIEW_H_
