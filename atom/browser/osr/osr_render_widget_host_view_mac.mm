// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_render_widget_host_view.h"

#import <Cocoa/Cocoa.h>

#include "base/strings/utf_string_conversions.h"
#include "content/common/view_messages.h"
#include "ui/accelerated_widget_mac/accelerated_widget_mac.h"

namespace atom {

class MacHelper :
    public content::BrowserCompositorMacClient,
    public ui::AcceleratedWidgetMacNSView {
 public:
  explicit MacHelper(OffScreenRenderWidgetHostView* view) : view_(view) {}
  virtual ~MacHelper() {}

  // content::BrowserCompositorMacClient:
  NSView* BrowserCompositorMacGetNSView() const override {
    // Intentionally return nil so that
    // BrowserCompositorMac::DelegatedFrameHostDesiredSizeInDIP uses the layer
    // size instead of the NSView size.
    return nil;
  }

  SkColor BrowserCompositorMacGetGutterColor(SkColor color) const override {
    // When making an element on the page fullscreen the element's background
    // may not match the page's, so use black as the gutter color to avoid
    // flashes of brighter colors during the transition.
    if (view_->render_widget_host()->delegate() &&
        view_->render_widget_host()->delegate()->IsFullscreenForCurrentTab()) {
      return SK_ColorBLACK;
    }
    return color;
  }

  void BrowserCompositorMacSendCompositorSwapAck(
      int output_surface_id,
      const cc::CompositorFrameAck& ack) override {
    view_->render_widget_host()->Send(new ViewMsg_SwapCompositorFrameAck(
        view_->render_widget_host()->GetRoutingID(), output_surface_id, ack));
  }

  void BrowserCompositorMacSendReclaimCompositorResources(
      int output_surface_id,
      const cc::CompositorFrameAck& ack) override {
    view_->render_widget_host()->Send(new ViewMsg_ReclaimCompositorResources(
        view_->render_widget_host()->GetRoutingID(), output_surface_id, ack));
  }

  void BrowserCompositorMacOnLostCompositorResources() override {
    view_->render_widget_host()->ScheduleComposite();
  }

  void BrowserCompositorMacUpdateVSyncParameters(
      const base::TimeTicks& timebase,
      const base::TimeDelta& interval) override {
    view_->render_widget_host()->UpdateVSyncParameters(timebase, interval);
  }

  void BrowserCompositorMacSendBeginFrame(
      const cc::BeginFrameArgs& args) override {
    view_->render_widget_host()->Send(
      new ViewMsg_BeginFrame(view_->render_widget_host()->GetRoutingID(),
                             args));
  }
  // ui::AcceleratedWidgetMacNSView:
  NSView* AcceleratedWidgetGetNSView() const override {
    return [view_->window()->GetNativeWindow() contentView];
  }

  void AcceleratedWidgetGetVSyncParameters(
        base::TimeTicks* timebase, base::TimeDelta* interval) const override {
    *timebase = base::TimeTicks();
    *interval = base::TimeDelta();
  }

  void AcceleratedWidgetSwapCompleted() override {
  }

 private:
  OffScreenRenderWidgetHostView* view_;

  DISALLOW_COPY_AND_ASSIGN(MacHelper);
};

ui::AcceleratedWidgetMac*
OffScreenRenderWidgetHostView::GetAcceleratedWidgetMac() const {
  if (browser_compositor_)
    return browser_compositor_->GetAcceleratedWidgetMac();
  return nullptr;
}

void OffScreenRenderWidgetHostView::SetActive(bool active) {
}

void OffScreenRenderWidgetHostView::ShowDefinitionForSelection() {
}

bool OffScreenRenderWidgetHostView::SupportsSpeech() const {
  return false;
}

void OffScreenRenderWidgetHostView::SpeakSelection() {
}

bool OffScreenRenderWidgetHostView::IsSpeaking() const {
  return false;
}

void OffScreenRenderWidgetHostView::StopSpeaking() {
}

void OffScreenRenderWidgetHostView::SelectionChanged(
    const base::string16& text,
    size_t offset,
    const gfx::Range& range) {
  if (range.is_empty() || text.empty()) {
    selected_text_.clear();
  } else {
    size_t pos = range.GetMin() - offset;
    size_t n = range.length();

    DCHECK(pos + n <= text.length()) << "The text can not fully cover range.";
    if (pos >= text.length()) {
      DCHECK(false) << "The text can not cover range.";
      return;
    }
    selected_text_ = base::UTF16ToUTF8(text.substr(pos, n));
  }

  RenderWidgetHostViewBase::SelectionChanged(text, offset, range);
}

void OffScreenRenderWidgetHostView::CreatePlatformWidget() {
  mac_helper_ = new MacHelper(this);
  browser_compositor_.reset(new content::BrowserCompositorMac(
      mac_helper_, mac_helper_, render_widget_host_->is_hidden(), true));
}

void OffScreenRenderWidgetHostView::DestroyPlatformWidget() {
  browser_compositor_.reset();
  delete mac_helper_;
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

}   // namespace
