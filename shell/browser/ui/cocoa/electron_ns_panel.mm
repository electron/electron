// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/electron_ns_panel.h"

// AppKit does not expose a public API for changing this after a window has
// been created. macOS 27 no longer honors the nonactivating panel style-mask
// workaround below when deciding whether a mouse click activates the app.
@interface NSWindow (ElectronPanelActivation)
- (void)_setPreventsActivation:(BOOL)preventsActivation;
@end

@implementation ElectronNSPanel

@synthesize originalStyleMask;

- (id)initWithShell:(electron::NativeWindowMac*)shell
          styleMask:(NSUInteger)styleMask {
  if (self = [super initWithShell:shell styleMask:styleMask]) {
    originalStyleMask = styleMask;
    if (@available(macOS 27.0, *)) {
      [self _setPreventsActivation:YES];
    }
  }
  return self;
}

@dynamic styleMask;
// The Nonactivating mask is reserved for NSPanel,
// but we can use this workaround to add it at runtime
- (NSWindowStyleMask)styleMask {
  return originalStyleMask | NSWindowStyleMaskNonactivatingPanel;
}

- (void)setStyleMask:(NSWindowStyleMask)styleMask {
  originalStyleMask = styleMask;
  // Notify change of style mask.
  [super setStyleMask:styleMask];
}

- (void)setCollectionBehavior:(NSWindowCollectionBehavior)collectionBehavior {
  NSWindowCollectionBehavior panelBehavior =
      (NSWindowCollectionBehaviorCanJoinAllSpaces |
       NSWindowCollectionBehaviorFullScreenAuxiliary);
  [super setCollectionBehavior:collectionBehavior | panelBehavior];
}

@end
