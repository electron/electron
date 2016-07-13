// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_OSR_WINDOW_H_
#define ATOM_BROWSER_OSR_WINDOW_H_

#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/renderer_host/delegated_frame_host.h"
#include "content/browser/renderer_host/resize_lock.h"
#include "third_party/WebKit/public/platform/WebVector.h"
#include "cc/scheduler/begin_frame_source.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "cc/output/compositor_frame.h"
#include "ui/gfx/geometry/point.h"
#include "base/threading/thread.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/compositor/layer_owner.h"
#include "ui/base/ime/text_input_client.h"
#include "base/process/kill.h"

#include "cc/output/software_output_device.h"
#include "cc/output/output_surface.h"
#include "cc/output/output_surface_client.h"
#include "cc/scheduler/begin_frame_source.h"

#include <windows.h>
#include "ui/gfx/win/window_impl.h"
#include "content/browser/web_contents/web_contents_view.h"
#include "atom/browser/native_window_views.h"

namespace atom {

class OffScreenWebContentsView : public content::WebContentsView {
public:
  OffScreenWebContentsView();
  ~OffScreenWebContentsView();

  gfx::NativeView GetNativeView() const;
  gfx::NativeView GetContentNativeView() const;
  gfx::NativeWindow GetTopLevelNativeWindow() const;

  void GetContainerBounds(gfx::Rect* out) const;
  void SizeContents(const gfx::Size& size);
  void Focus();
  void SetInitialFocus();
  void StoreFocus();
  void RestoreFocus();
  content::DropData* GetDropData() const;
  gfx::Rect GetViewBounds() const;

  void CreateView(
      const gfx::Size& initial_size, gfx::NativeView context);

  content::RenderWidgetHostViewBase* CreateViewForWidget(
      content::RenderWidgetHost* render_widget_host, bool is_guest_view_hack);
  content::RenderWidgetHostViewBase* CreateViewForPopupWidget(
      content::RenderWidgetHost* render_widget_host);

  void SetPageTitle(const base::string16& title);
  void RenderViewCreated(content::RenderViewHost* host);
  void RenderViewSwappedIn(content::RenderViewHost* host);
  void SetOverscrollControllerEnabled(bool enabled);
private:
  content::RenderWidgetHostViewBase* view_;
};

class OffScreenOutputDevice : public cc::SoftwareOutputDevice {
public:
  OffScreenOutputDevice();
  ~OffScreenOutputDevice();


  void saveSkBitmapToBMPFile(const SkBitmap& skBitmap, const char* path);
  void Resize(const gfx::Size& pixel_size, float scale_factor) override;
  SkCanvas* BeginPaint(const gfx::Rect& damage_rect) override;
  void EndPaint() override;

private:
  std::unique_ptr<SkCanvas> canvas_;
  std::unique_ptr<SkBitmap> bitmap_;
  gfx::Rect pending_damage_rect_;

  DISALLOW_COPY_AND_ASSIGN(OffScreenOutputDevice);
};

class AtomCompositorDelegate : public ui::CompositorDelegate {
public:
  AtomCompositorDelegate() {};
  ~AtomCompositorDelegate() {};

  std::unique_ptr<cc::SoftwareOutputDevice> CreateSoftwareOutputDevice(
      ui::Compositor* compositor) {
    return std::unique_ptr<cc::SoftwareOutputDevice>(new OffScreenOutputDevice);
  }
};

class OffScreenWindow
    : public content::RenderWidgetHostViewBase,
      public content::DelegatedFrameHostClient,
      public ui::CompositorDelegate {
public:
  OffScreenWindow(content::RenderWidgetHost*);
  ~OffScreenWindow();

  void CreatePlatformWidget();

//content::RenderWidgetHostView
  bool OnMessageReceived(const IPC::Message&) override;
  void InitAsChild(gfx::NativeView);
  content::RenderWidgetHost* GetRenderWidgetHost(void) const;
  void SetSize(const gfx::Size &);
  void SetBounds(const gfx::Rect &);
  gfx::Vector2dF GetLastScrollOffset(void) const;
  gfx::NativeView GetNativeView(void) const;
  gfx::NativeViewId GetNativeViewId(void) const;
  gfx::NativeViewAccessible GetNativeViewAccessible(void);
  ui::TextInputClient* GetTextInputClient() override;
  void Focus(void);
  bool HasFocus(void) const;
  bool IsSurfaceAvailableForCopy(void) const;
  void Show(void);
  void Hide(void);
  bool IsShowing(void);
  gfx::Rect GetViewBounds(void) const;
  gfx::Size GetVisibleViewportSize() const override;
  void SetInsets(const gfx::Insets&) override;
  bool LockMouse(void);
  void UnlockMouse(void);
  bool GetScreenColorProfile(std::vector<char>*);

//content::RenderWidgetHostViewBase
  void OnSwapCompositorFrame(uint32_t, std::unique_ptr<cc::CompositorFrame>);
  void ClearCompositorFrame(void);
  void InitAsPopup(content::RenderWidgetHostView *, const gfx::Rect &);
  void InitAsFullscreen(content::RenderWidgetHostView *);
  void UpdateCursor(const content::WebCursor &);
  void SetIsLoading(bool);
  void TextInputStateChanged(const ViewHostMsg_TextInputState_Params &);
  void ImeCancelComposition(void);
  void RenderProcessGone(base::TerminationStatus,int);
  void Destroy(void);
  void SetTooltipText(const base::string16 &);
  void SelectionBoundsChanged(const ViewHostMsg_SelectionBounds_Params &);
  void CopyFromCompositingSurface(const gfx::Rect &,
    const gfx::Size &,
    const content::ReadbackRequestCallback &,
    const SkColorType);
  void CopyFromCompositingSurfaceToVideoFrame(
    const gfx::Rect &,
    const scoped_refptr<media::VideoFrame> &,
    const base::Callback<void (const gfx::Rect &, bool),
    base::internal::CopyMode::Copyable> &);
  bool CanCopyToVideoFrame(void) const;
  void BeginFrameSubscription(
    std::unique_ptr<content::RenderWidgetHostViewFrameSubscriber>);
  void EndFrameSubscription();
  bool HasAcceleratedSurface(const gfx::Size &);
  void GetScreenInfo(blink::WebScreenInfo *);
  bool GetScreenColorProfile(blink::WebVector<char>*);
  gfx::Rect GetBoundsInRootWindow(void);
  void LockCompositingSurface(void);
  void UnlockCompositingSurface(void);
  void ImeCompositionRangeChanged(
    const gfx::Range &, const std::vector<gfx::Rect>&);
  gfx::Size GetPhysicalBackingSize() const override;
  gfx::Size GetRequestedRendererSize() const override;

//content::DelegatedFrameHostClient
  int DelegatedFrameHostGetGpuMemoryBufferClientId(void) const;
  ui::Layer *DelegatedFrameHostGetLayer(void) const;
  bool DelegatedFrameHostIsVisible(void) const;
  SkColor DelegatedFrameHostGetGutterColor(SkColor) const;
  gfx::Size DelegatedFrameHostDesiredSizeInDIP(void) const;
  bool DelegatedFrameCanCreateResizeLock(void) const;
  std::unique_ptr<content::ResizeLock> DelegatedFrameHostCreateResizeLock(bool);
  void DelegatedFrameHostResizeLockWasReleased(void);
  void DelegatedFrameHostSendCompositorSwapAck(
    int, const cc::CompositorFrameAck &);
  void DelegatedFrameHostSendReclaimCompositorResources(
    int, const cc::CompositorFrameAck &);
  void DelegatedFrameHostOnLostCompositorResources(void);
  void DelegatedFrameHostUpdateVSyncParameters(
    const base::TimeTicks &, const base::TimeDelta &);

  std::unique_ptr<cc::SoftwareOutputDevice> CreateSoftwareOutputDevice(
      ui::Compositor* compositor);
  void OnSetNeedsBeginFrames(bool enabled);
private:
  content::RenderWidgetHostImpl* render_widget_host_;

  std::unique_ptr<content::DelegatedFrameHost> delegated_frame_host_;
  std::unique_ptr<ui::Compositor> compositor_;
  gfx::AcceleratedWidget compositor_widget_;
  std::unique_ptr<ui::Layer> root_layer_;

  float scale_factor_;
  bool is_showing_;
  gfx::Vector2dF last_scroll_offset_;
  gfx::Size size_;

#if defined(OS_WIN)
  std::unique_ptr<gfx::WindowImpl> window_;
#endif

  base::WeakPtrFactory<OffScreenWindow> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(OffScreenWindow);
};

}  // namespace atom

#endif  // ATOM_BROWSER_OSR_WINDOW_H_
