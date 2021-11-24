// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_H_

#include "components/remote_cocoa/app_shim/native_widget_mac_nswindow.h"
#include "shell/browser/ui/cocoa/event_dispatching_window.h"
#include "ui/views/widget/native_widget_mac.h"

namespace electron {

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

}  // namespace electron

@interface ElectronNSWindow : NativeWidgetMacNSWindow {
 @private
  electron::NativeWindowMac* shell_;
}
@property BOOL acceptsFirstMouse;
@property BOOL enableLargerThanScreen;
@property BOOL disableAutoHideCursor;
@property BOOL disableKeyOrMainWindow;
@property(nonatomic, retain) NSVisualEffectView* vibrantView;
@property(nonatomic, retain) NSImage* cornerMask;
- (id)initWithShell:(electron::NativeWindowMac*)shell
          styleMask:(NSUInteger)styleMask;
- (electron::NativeWindowMac*)shell;
- (id)accessibilityFocusedUIElement;
- (NSRect)originalContentRectForFrameRect:(NSRect)frameRect;
- (void)toggleFullScreenMode:(id)sender;
- (NSImage*)_cornerMask;
@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_H_
