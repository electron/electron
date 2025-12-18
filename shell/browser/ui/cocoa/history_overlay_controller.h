// Copyright (c) 2024 Electron.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_HISTORY_OVERLAY_CONTROLLER_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_HISTORY_OVERLAY_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

namespace electron {
enum HistoryOverlayMode { kHistoryOverlayModeBack, kHistoryOverlayModeForward };
}

@interface HistoryOverlayController : NSViewController

- (instancetype)initForMode:(electron::HistoryOverlayMode)mode;
- (void)showPanelForView:(NSView*)view;
- (void)setProgress:(CGFloat)gestureAmount finished:(BOOL)finished;
- (void)dismiss;

@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_HISTORY_OVERLAY_CONTROLLER_H_
