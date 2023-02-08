// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_PANEL_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_PANEL_H_

#include "shell/browser/ui/cocoa/electron_ns_window.h"

@interface ElectronNSPanel : ElectronNSWindow
@property NSWindowStyleMask styleMask;
@property NSWindowStyleMask originalStyleMask;
- (id)initWithShell:(electron::NativeWindowMac*)shell
          styleMask:(NSUInteger)styleMask;
@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NS_PANEL_H_
