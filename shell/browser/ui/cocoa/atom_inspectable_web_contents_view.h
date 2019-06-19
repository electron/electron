// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ATOM_BROWSER_UI_COCOA_BRY_INSPECTABLE_WEB_CONTENTS_VIEW_H_
#define ATOM_BROWSER_UI_COCOA_BRY_INSPECTABLE_WEB_CONTENTS_VIEW_H_

#import <AppKit/AppKit.h>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/devtools/devtools_contents_resizing_strategy.h"
#include "ui/base/cocoa/base_view.h"

namespace atom {
class InspectableWebContentsViewMac;
}

using atom::InspectableWebContentsViewMac;

@interface AtomInspectableWebContentsView : BaseView <NSWindowDelegate> {
 @private
  atom::InspectableWebContentsViewMac* inspectableWebContentsView_;

  base::scoped_nsobject<NSView> fake_view_;
  base::scoped_nsobject<NSWindow> devtools_window_;
  BOOL devtools_visible_;
  BOOL devtools_docked_;
  BOOL devtools_is_first_responder_;

  DevToolsContentsResizingStrategy strategy_;
}

- (instancetype)initWithInspectableWebContentsViewMac:
    (InspectableWebContentsViewMac*)view;
- (void)removeObservers;
- (void)notifyDevToolsFocused;
- (void)setDevToolsVisible:(BOOL)visible activate:(BOOL)activate;
- (BOOL)isDevToolsVisible;
- (BOOL)isDevToolsFocused;
- (void)setIsDocked:(BOOL)docked activate:(BOOL)activate;
- (void)setContentsResizingStrategy:
    (const DevToolsContentsResizingStrategy&)strategy;
- (void)setTitle:(NSString*)title;

@end

#endif  // ATOM_BROWSER_UI_COCOA_BRY_INSPECTABLE_WEB_CONTENTS_VIEW_H_
