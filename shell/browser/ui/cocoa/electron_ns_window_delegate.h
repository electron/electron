// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_DELEGATE_H_

#include <Quartz/Quartz.h>

#include <optional>

#include "base/memory/raw_ptr.h"
#include "components/remote_cocoa/app_shim/views_nswindow_delegate.h"

namespace electron {
class NativeWindowMac;
}

@interface ElectronNSWindowDelegate
    : ViewsNSWindowDelegate <NSTouchBarDelegate, QLPreviewPanelDataSource> {
 @private
  raw_ptr<electron::NativeWindowMac> shell_;
  bool is_zooming_;
  int level_;
  bool is_resizable_;

  // Whether the window is currently minimized. Used to work
  // around a macOS bug with child window minimization.
  bool is_minimized_;

  // Only valid during a live resize.
  // Used to keep track of whether a resize is happening horizontally or
  // vertically, even if physically the user is resizing in both directions.
  std::optional<bool> resizingHorizontally_;
}
- (id)initWithShell:(electron::NativeWindowMac*)shell;
@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_DELEGATE_H_
