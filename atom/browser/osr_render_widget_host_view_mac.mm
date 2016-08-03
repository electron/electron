// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr_render_widget_host_view.h"

#import <Cocoa/Cocoa.h>

#include "base/strings/utf_string_conversions.h"
#include "ui/accelerated_widget_mac/accelerated_widget_mac.h"

ui::AcceleratedWidgetMac*
  atom::OffScreenRenderWidgetHostView::GetAcceleratedWidgetMac()
    const {
  if (browser_compositor_)
    return browser_compositor_->accelerated_widget_mac();
  return nullptr;
}

NSView* atom::OffScreenRenderWidgetHostView::AcceleratedWidgetGetNSView()
    const {
  return [window_ contentView];
}

void atom::OffScreenRenderWidgetHostView::AcceleratedWidgetGetVSyncParameters(
    base::TimeTicks* timebase, base::TimeDelta* interval) const {
  *timebase = base::TimeTicks();
  *interval = base::TimeDelta();
}

void atom::OffScreenRenderWidgetHostView::AcceleratedWidgetSwapCompleted() {
}

void atom::OffScreenRenderWidgetHostView::SetActive(bool active) {
}

void atom::OffScreenRenderWidgetHostView::ShowDefinitionForSelection() {
}

bool atom::OffScreenRenderWidgetHostView::SupportsSpeech() const {
  return false;
}

void atom::OffScreenRenderWidgetHostView::SpeakSelection() {
}

bool atom::OffScreenRenderWidgetHostView::IsSpeaking() const {
  return false;
}

void atom::OffScreenRenderWidgetHostView::StopSpeaking() {
}

void atom::OffScreenRenderWidgetHostView::SelectionChanged(
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

void atom::OffScreenRenderWidgetHostView::CreatePlatformWidget() {
  window_ = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1, 1)
                                        styleMask:NSBorderlessWindowMask
                                          backing:NSBackingStoreBuffered
                                            defer:NO];

  background_layer_ = [[[CALayer alloc] init] retain];
  [background_layer_ setBackgroundColor:CGColorGetConstantColor(kCGColorClear)];
  NSView* content_view = [window_ contentView];
  [content_view setLayer:background_layer_];
  [content_view setWantsLayer:YES];

  browser_compositor_ = content::BrowserCompositorMac::Create();

  compositor_.reset(browser_compositor_->compositor());
  compositor_->SetRootLayer(root_layer_.get());
  browser_compositor_->accelerated_widget_mac()->SetNSView(this);
  browser_compositor_->compositor()->SetVisible(true);

  compositor_->SetLocksWillTimeOut(true);
  browser_compositor_->Unsuspend();
}

void atom::OffScreenRenderWidgetHostView::DestroyPlatformWidget() {
  DCHECK(window_);

  ui::Compositor* compositor = compositor_.release();
  ALLOW_UNUSED_LOCAL(compositor);

  [window_ close];
  window_ = nil;
  [background_layer_ release];
  background_layer_ = nil;

  browser_compositor_->accelerated_widget_mac()->ResetNSView();
  browser_compositor_->compositor()->SetVisible(false);
  browser_compositor_->compositor()->SetScaleAndSize(1.0, gfx::Size(0, 0));
  browser_compositor_->compositor()->SetRootLayer(NULL);
  content::BrowserCompositorMac::Recycle(std::move(browser_compositor_));
}
