// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_SHIM_APP_SHIM_WINDOW_H_
#define ELECTRON_SHELL_APP_SHIM_APP_SHIM_WINDOW_H_

#import <Cocoa/Cocoa.h>
#import <Quartz/Quartz.h>

#include <optional>
#include <string>

#include "electron/shell/common/app_shim.mojom.h"
#include "ui/gfx/geometry/point.h"

// The parts of a window in an app shim that the host app asks for and that
// remote_cocoa has no call for: the window buttons of a hidden title bar,
// vibrancy and the Quick Look preview. What NativeWindowMac does to its own
// NSWindow for a window of the host's process, this does to the NSWindow that
// remote_cocoa made here.
@interface ElectronAppShimWindow
    : NSObject <QLPreviewPanelDataSource, QLPreviewPanelDelegate>

// The object for the window that the host knows by `windowId`, made on first
// use. Nil when there is no such window (any more).
+ (instancetype)forWindowId:(uint64_t)windowId;
// The object for `window`, if one has been made.
+ (instancetype)existingForWindow:(NSWindow*)window;

- (void)initChrome:(const electron::mojom::AppShimWindowChrome&)chrome;
- (void)setButtonVisibility:(BOOL)visible;
- (void)setButtonPosition:(const std::optional<gfx::Point>&)position;
- (void)setVibrancy:(const std::string&)type
           duration:(int)durationMs
              state:(NSVisualEffectState)state;
- (void)performTabAction:(electron::mojom::AppShimTabAction)action;
- (void)addTabbedWindow:(ElectronAppShimWindow*)other;
- (void)previewFile:(NSURL*)url title:(NSString*)title;
- (void)closeFilePreview;
// Whether the Quick Look panel is this window's to fill.
- (BOOL)hasPreviewItem;

@end

#endif  // ELECTRON_SHELL_APP_SHIM_APP_SHIM_WINDOW_H_
