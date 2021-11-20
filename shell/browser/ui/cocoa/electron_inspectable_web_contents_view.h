// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef SHELL_BROWSER_UI_COCOA_ELECTRON_INSPECTABLE_WEB_CONTENTS_VIEW_H_
#define SHELL_BROWSER_UI_COCOA_ELECTRON_INSPECTABLE_WEB_CONTENTS_VIEW_H_

#import <AppKit/AppKit.h>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/devtools/devtools_contents_resizing_strategy.h"
#include "ui/base/cocoa/base_view.h"

namespace electron {
class InspectableWebContentsViewMac;
}

using electron::InspectableWebContentsViewMac;

@interface NSView (WebContentsView)
- (void)setMouseDownCanMoveWindow:(BOOL)can_move;
@end

@interface ControlRegionView : NSView
@end

@interface ElectronInspectableWebContentsView : BaseView <NSWindowDelegate> {
 @private
  electron::InspectableWebContentsViewMac* inspectableWebContentsView_;

  base::scoped_nsobject<NSView> fake_view_;
  base::scoped_nsobject<NSWindow> devtools_window_;
  base::scoped_nsobject<ControlRegionView> devtools_mask_;
  BOOL devtools_visible_;
  BOOL devtools_docked_;
  BOOL devtools_is_first_responder_;
  BOOL attached_to_window_;

  DevToolsContentsResizingStrategy strategy_;
}

- (instancetype)initWithInspectableWebContentsViewMac:
    (InspectableWebContentsViewMac*)view;
- (void)notifyDevToolsFocused;
- (void)setDevToolsVisible:(BOOL)visible activate:(BOOL)activate;
- (BOOL)isDevToolsVisible;
- (BOOL)isDevToolsFocused;
- (void)setIsDocked:(BOOL)docked activate:(BOOL)activate;
- (void)setContentsResizingStrategy:
    (const DevToolsContentsResizingStrategy&)strategy;
- (void)setTitle:(NSString*)title;

@end

#endif  // SHELL_BROWSER_UI_COCOA_ELECTRON_INSPECTABLE_WEB_CONTENTS_VIEW_H_
