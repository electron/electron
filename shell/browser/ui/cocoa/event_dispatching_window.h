// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_EVENT_DISPATCHING_WINDOW_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_EVENT_DISPATCHING_WINDOW_H_

#import "ui/base/cocoa/underlay_opengl_hosting_window.h"

@interface EventDispatchingWindow : UnderlayOpenGLHostingWindow {
 @private
  BOOL redispatchingEvent_;
}

- (void)redispatchKeyEvent:(NSEvent*)event;

@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_EVENT_DISPATCHING_WINDOW_H_
