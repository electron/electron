// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_OSR_OSR_WEB_CONTENTS_VIEW_H_
#define ELECTRON_SHELL_BROWSER_OSR_OSR_WEB_CONTENTS_VIEW_H_

#include <memory>
#include <optional>

#include "shell/browser/native_window_observer.h"

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ptr_exclusion.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/renderer_host/render_view_host_delegate_view.h"  // nogncheck
#include "content/browser/web_contents/web_contents_view.h"  // nogncheck
#include "content/browser/web_contents/web_contents_view_drag_security_info.h"  // nogncheck
#include "shell/browser/osr/osr_render_widget_host_view.h"
#include "third_party/blink/public/common/page/drag_operation.h"
#include "third_party/blink/public/mojom/drag/drag.mojom-forward.h"
#include "ui/base/dragdrop/mojom/drag_drop_types.mojom-shared.h"
#include "ui/gfx/geometry/point_f.h"

#if BUILDFLAG(IS_MAC)
#ifdef __OBJC__
@class OffScreenView;
#else
class OffScreenView;
#endif
#endif

namespace blink {
class WebKeyboardEvent;
class WebMouseEvent;
}  // namespace blink

namespace content {
class WebContents;
}

namespace electron {

class NativeWindow;
class OffScreenDragDelegate;

class OffScreenWebContentsView : public content::WebContentsView,
                                 public content::RenderViewHostDelegateView,
                                 private NativeWindowObserver {
 public:
  OffScreenWebContentsView(
      bool transparent,
      bool offscreen_use_shared_texture,
      const std::string& offscreen_shared_texture_pixel_format,
      float offscreen_device_scale_factor,
      const OnPaintCallback& callback);
  ~OffScreenWebContentsView() override;

  // Returns the view of |web_contents| if it is an OffScreenWebContentsView.
  static OffScreenWebContentsView* FromWebContents(
      content::WebContents* web_contents);

  void SetWebContents(content::WebContents*);
  void SetNativeWindow(NativeWindow* window);
  void SetCallback(const OnPaintCallback& callback);
  void SetDragDelegate(OffScreenDragDelegate* delegate);

  // Renderer-initiated drag and drop, driven by embedder input events. Every
  // embedder mouse event must pass through HandleDragMouseEvent(); both return
  // true if |event| was consumed by an in-progress drag.
  bool HandleDragMouseEvent(const blink::WebMouseEvent& event);
  bool HandleDragKeyEvent(const blink::WebKeyboardEvent& event);
  void CancelDrag();

  // NativeWindowObserver:
  void OnWindowResize() override;
  void OnWindowClosed() override;

  gfx::Size GetSize() const override;

  // content::WebContentsView:
  gfx::NativeView GetNativeView() const override;
  gfx::NativeView GetContentNativeView() const override;
  gfx::NativeWindow GetTopLevelNativeWindow() const override;
  gfx::Rect GetContainerBounds() const override;
  void Focus() override {}
  void Resize(const gfx::Rect& new_bounds) override {}
  void SetInitialFocus() override {}
  void StoreFocus() override {}
  void RestoreFocus() override {}
  void FocusThroughTabTraversal(bool reverse) override {}
  content::DropData* GetDropData() const override;
  gfx::Rect GetViewBounds() const override;
  void CreateView(gfx::NativeView context) override {}
  content::RenderWidgetHostViewBase* CreateViewForWidget(
      content::RenderWidgetHost* render_widget_host) override;
  content::RenderWidgetHostViewBase* CreateViewForChildWidget(
      content::RenderWidgetHost* render_widget_host) override;
  void SetPageTitle(const std::u16string& title) override {}
  void RenderViewReady() override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override {}
  void SetOverscrollControllerEnabled(bool enabled) override {}
  void OnCapturerCountChanged() override {}
  void FullscreenStateChanged(bool is_fullscreen) override {}
  void UpdateWindowControlsOverlay(const gfx::Rect& bounding_rect) override {}
  content::BackForwardTransitionAnimationManager*
  GetBackForwardTransitionAnimationManager() override;
  void DestroyBackForwardTransitionAnimationManager() override {}

#if BUILDFLAG(IS_MAC)
  bool CloseTabAfterEventTrackingIfNeeded() override;
#endif

  // content::RenderViewHostDelegateView
  void StartDragging(
      content::RenderFrameHost& source_rfh,
      const content::DropData& drop_data,
      blink::DragOperationsMask allowed_ops,
      const gfx::ImageSkia& image,
      const gfx::Vector2d& cursor_offset,
      const gfx::Rect& drag_obj_rect,
      const blink::mojom::DragEventSourceInfo& event_info) override;
  void UpdateDragOperation(ui::mojom::DragOperation operation,
                           bool document_is_handling_drag) override {}
  void SetPainting(bool painting);
  bool IsPainting() const;
  void SetFrameRate(int frame_rate);
  int GetFrameRate() const;

 private:
#if BUILDFLAG(IS_MAC)
  void PlatformCreate();
  void PlatformDestroy();
#endif

  OffScreenRenderWidgetHostView* GetView() const;

  struct DragState {
    DragState();
    DragState(DragState&&);
    ~DragState();

    std::unique_ptr<content::DropData> drop_data;
    blink::DragOperationsMask allowed_ops = blink::kDragOperationNone;
    base::WeakPtr<content::RenderWidgetHostImpl> source_rwh;
    base::WeakPtr<content::RenderWidgetHostImpl> target_rwh;
    ui::mojom::DragOperation operation = ui::mojom::DragOperation::kNone;
  };

  void RefuseDrag(content::RenderWidgetHostImpl* source_rwh);
  content::RenderWidgetHostImpl* GetDragTargetWidget() const;
  void DragTargetUpdate(const blink::WebMouseEvent& event);
  void DragTargetLeave();
  void OnDragOperationNegotiated(ui::mojom::DragOperation operation,
                                 bool document_is_handling_drag);
  void SetDragOperation(ui::mojom::DragOperation operation);
  // |release_mouse| tells Blink the button is up, since the drag swallowed
  // the real release.
  void EndDrag(ui::mojom::DragOperation operation,
               bool cancelled,
               bool release_mouse);
  void SendMouseReleasedMove();

  raw_ptr<NativeWindow> native_window_ = nullptr;
  raw_ptr<OffScreenDragDelegate> drag_delegate_ = nullptr;

  // Last embedder mouse state seen by HandleDragMouseEvent().
  bool left_button_down_ = false;
  gfx::PointF last_client_pt_;
  gfx::PointF last_screen_pt_;

  std::optional<DragState> drag_;
  content::WebContentsViewDragSecurityInfo drag_security_info_;

  const bool transparent_;
  const bool offscreen_use_shared_texture_;
  const std::string offscreen_shared_texture_pixel_format_;
  const float offscreen_device_scale_factor_;
  bool painting_ = true;
  int frame_rate_ = 60;
  OnPaintCallback callback_;

  // Weak refs.
  raw_ptr<content::WebContents> web_contents_ = nullptr;

#if BUILDFLAG(IS_MAC)
  RAW_PTR_EXCLUSION OffScreenView* offScreenView_ = nullptr;
#endif

  base::WeakPtrFactory<OffScreenWebContentsView> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_OSR_OSR_WEB_CONTENTS_VIEW_H_
