// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_COCOA_ATOM_NS_WINDOW_H_
#define ATOM_BROWSER_UI_COCOA_ATOM_NS_WINDOW_H_

#include "atom/browser/ui/cocoa/event_dispatching_window.h"
#include "ui/views/widget/native_widget_mac.h"
#include "ui/views_bridge_mac/native_widget_mac_nswindow.h"

namespace atom {

class NativeWindowMac;

// Prevents window from resizing during the scope.
class ScopedDisableResize {
 public:
  ScopedDisableResize() { disable_resize_ = true; }
  ~ScopedDisableResize() { disable_resize_ = false; }

  static bool IsResizeDisabled() { return disable_resize_; }

 private:
  static bool disable_resize_;
};

}  // namespace atom

@interface AtomNSWindow : NativeWidgetMacNSWindow {
 @private
  atom::NativeWindowMac* shell_;
}
@property BOOL acceptsFirstMouse;
@property BOOL enableLargerThanScreen;
@property BOOL disableAutoHideCursor;
@property BOOL disableKeyOrMainWindow;
@property(nonatomic, retain) NSView* vibrantView;
- (id)initWithShell:(atom::NativeWindowMac*)shell
          styleMask:(NSUInteger)styleMask;
- (atom::NativeWindowMac*)shell;
- (id)accessibilityFocusedUIElement;
- (NSRect)originalContentRectForFrameRect:(NSRect)frameRect;
- (void)toggleFullScreenMode:(id)sender;
@end

#endif  // ATOM_BROWSER_UI_COCOA_ATOM_NS_WINDOW_H_
