// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_DELEGATE_H_

#include <Quartz/Quartz.h>

#include "components/remote_cocoa/app_shim/views_nswindow_delegate.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace electron {
class NativeWindowMac;
}

@interface ElectronNSWindowDelegate
    : ViewsNSWindowDelegate <NSTouchBarDelegate, QLPreviewPanelDataSource> {
 @private
  electron::NativeWindowMac* shell_;
  bool is_zooming_;
  int level_;
  bool is_resizable_;

  // Only valid during a live resize.
  // Used to keep track of whether a resize is happening horizontally or
  // vertically, even if physically the user is resizing in both directions.
  absl::optional<bool> resizingHorizontally_;
}
- (id)initWithShell:(electron::NativeWindowMac*)shell;
@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_DELEGATE_H_
