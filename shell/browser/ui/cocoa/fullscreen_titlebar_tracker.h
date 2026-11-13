// Copyright (c) 2026 Electron contributors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_FULLSCREEN_TITLEBAR_TRACKER_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_FULLSCREEN_TITLEBAR_TRACKER_H_

#import <Cocoa/Cocoa.h>

#include "base/functional/callback.h"

@class WindowButtonsProxy;

// Observes AppKit's fullscreen titlebar reveal and native frame.
// This class is not compiled in MAS builds.
@interface FullscreenTitlebarTracker : NSObject
- (instancetype)initWithWindow:(NSWindow*)window
                  buttonsProxy:(WindowButtonsProxy*)buttonsProxy
                      onChange:(base::RepeatingClosure)onChange;
- (void)enterFullscreen;
- (void)leaveFullscreen;
- (void)update;
- (BOOL)isActive;
- (NSView*)container;
// The native titlebar frame in screen coordinates.
- (NSRect)nativeFrame;
- (double)revealAmount;
- (void)stop;
@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_FULLSCREEN_TITLEBAR_TRACKER_H_
