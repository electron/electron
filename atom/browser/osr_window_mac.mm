#include "atom/browser/osr_window.h"

#include <algorithm>
#include <limits>
#include <utility>
#include <iostream>

#import <Cocoa/Cocoa.h>

#include "base/compiler_specific.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/view_messages.h"
#include "ui/accelerated_widget_mac/accelerated_widget_mac.h"
#include "ui/events/latency_info.h"

ui::AcceleratedWidgetMac* atom::OffScreenWindow::GetAcceleratedWidgetMac()
    const {
  if (browser_compositor_)
    return browser_compositor_->accelerated_widget_mac();
  return nullptr;
}

NSView* atom::OffScreenWindow::AcceleratedWidgetGetNSView() const {
  return [window_ contentView];
}

void atom::OffScreenWindow::AcceleratedWidgetGetVSyncParameters(
    base::TimeTicks* timebase, base::TimeDelta* interval) const {
  *timebase = base::TimeTicks();
  *interval = base::TimeDelta();
}

void atom::OffScreenWindow::AcceleratedWidgetSwapCompleted() {
}

void atom::OffScreenWindow::SetActive(bool active) {
}

void atom::OffScreenWindow::ShowDefinitionForSelection() {
}

bool atom::OffScreenWindow::SupportsSpeech() const {
  return false;
}

void atom::OffScreenWindow::SpeakSelection() {
}

bool atom::OffScreenWindow::IsSpeaking() const {
  return false;
}

void atom::OffScreenWindow::StopSpeaking() {
}

void atom::OffScreenWindow::SelectionChanged(
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

void atom::OffScreenWindow::CreatePlatformWidget() {
  // Create a borderless non-visible 1x1 window.
  window_ = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1, 1)
                                        styleMask:NSBorderlessWindowMask
                                          backing:NSBackingStoreBuffered
                                            defer:NO];

  // Create a CALayer which is used by BrowserCompositorViewMac for rendering.
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

  // CEF needs the browser compositor to remain responsive whereas normal
  // rendering on OS X does not. This effectively reverts the changes from
  // https://crbug.com/463988#c6
  compositor_->SetLocksWillTimeOut(true);
  browser_compositor_->Unsuspend();
}

// void atom::OffScreenWindow::PlatformDestroyCompositorWidget() {
//   DCHECK(window_);
//
//   browser_compositor_->Destroy();
//
//   [window_ close];
//   window_ = nil;
//   [background_layer_ release];
//   background_layer_ = nil;
//
//   browser_compositor_.reset();
//
//   delete accelerated_widget_helper_;
//   accelerated_widget_helper_ = nullptr;
// }
