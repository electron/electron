// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_render_widget_host_view.h"

#import <Cocoa/Cocoa.h>

#include "base/strings/utf_string_conversions.h"
#include "content/common/view_messages.h"
#include "ui/accelerated_widget_mac/accelerated_widget_mac.h"

namespace atom {

class MacHelper : public content::BrowserCompositorMacClient,
                  public ui::AcceleratedWidgetMacNSView {
 public:
  explicit MacHelper(OffScreenRenderWidgetHostView* view) : view_(view) {
    [this->AcceleratedWidgetGetNSView() setWantsLayer:YES];
  }

  virtual ~MacHelper() {}

  // content::BrowserCompositorMacClient:
  SkColor BrowserCompositorMacGetGutterColor() const override {
    // When making an element on the page fullscreen the element's background
    // may not match the page's, so use black as the gutter color to avoid
    // flashes of brighter colors during the transition.
    if (view_->render_widget_host()->delegate() &&
        view_->render_widget_host()->delegate()->IsFullscreenForCurrentTab()) {
      return SK_ColorBLACK;
    }
    return view_->last_frame_root_background_color();
  }

  void BrowserCompositorMacOnBeginFrame() override {}

  void OnFrameTokenChanged(uint32_t frame_token) override {
    view_->render_widget_host()->DidProcessFrame(frame_token);
  }

  // ui::AcceleratedWidgetMacNSView:
  NSView* AcceleratedWidgetGetNSView() const override {
    return [view_->window()->GetNativeWindow() contentView];
  }

  void AcceleratedWidgetGetVSyncParameters(
      base::TimeTicks* timebase,
      base::TimeDelta* interval) const override {
    *timebase = base::TimeTicks();
    *interval = base::TimeDelta();
  }

  void AcceleratedWidgetSwapCompleted() override {}

  void DidReceiveFirstFrameAfterNavigation() override {
    view_->render_widget_host()->DidReceiveFirstFrameAfterNavigation();
  }

  void DestroyCompositorForShutdown() override {}

 private:
  OffScreenRenderWidgetHostView* view_;

  DISALLOW_COPY_AND_ASSIGN(MacHelper);
};

void OffScreenRenderWidgetHostView::SetActive(bool active) {}

void OffScreenRenderWidgetHostView::ShowDefinitionForSelection() {}

bool OffScreenRenderWidgetHostView::SupportsSpeech() const {
  return false;
}

void OffScreenRenderWidgetHostView::SpeakSelection() {}

bool OffScreenRenderWidgetHostView::IsSpeaking() const {
  return false;
}

void OffScreenRenderWidgetHostView::StopSpeaking() {}

bool OffScreenRenderWidgetHostView::ShouldContinueToPauseForFrame() {
  return browser_compositor_->ShouldContinueToPauseForFrame();
}

void OffScreenRenderWidgetHostView::CreatePlatformWidget(
    bool is_guest_view_hack) {
  mac_helper_ = new MacHelper(this);
  browser_compositor_.reset(new content::BrowserCompositorMac(
      mac_helper_, mac_helper_, render_widget_host_->is_hidden(), true,
      AllocateFrameSinkId(is_guest_view_hack)));
}

void OffScreenRenderWidgetHostView::DestroyPlatformWidget() {
  browser_compositor_.reset();
  delete mac_helper_;
}

viz::LocalSurfaceId OffScreenRenderWidgetHostView::GetLocalSurfaceId() const {
  return browser_compositor_->GetRendererLocalSurfaceId();
}

ui::Compositor* OffScreenRenderWidgetHostView::GetCompositor() const {
  return browser_compositor_->GetCompositor();
}

ui::Layer* OffScreenRenderWidgetHostView::GetRootLayer() const {
  return browser_compositor_->GetRootLayer();
}

content::DelegatedFrameHost*
OffScreenRenderWidgetHostView::GetDelegatedFrameHost() const {
  return browser_compositor_->GetDelegatedFrameHost();
}

}  // namespace atom
