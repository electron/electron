// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_EVENT_DISPATCHING_WINDOW_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_EVENT_DISPATCHING_WINDOW_H_

#import <Cocoa/Cocoa.h>

@interface EventDispatchingWindow : NSWindow {
 @private
  BOOL redispatchingEvent_;
}

- (void)redispatchKeyEvent:(NSEvent*)event;

@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_EVENT_DISPATCHING_WINDOW_H_
