// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_COCOA_EVENT_PROCESSING_WINDOW_H_
#define ATOM_BROWSER_UI_COCOA_EVENT_PROCESSING_WINDOW_H_

#import <Cocoa/Cocoa.h>

// Override NSWindow to access unhandled keyboard events (for command
// processing); subclassing NSWindow is the only method to do
// this.
@interface EventProcessingWindow : NSWindow {
 @private
  BOOL redispatchingEvent_;
  BOOL eventHandled_;
}

// Sends a key event to |NSApp sendEvent:|, but also makes sure that it's not
// short-circuited to the RWHV. This is used to send keyboard events to the menu
// and the cmd-` handler if a keyboard event comes back unhandled from the
// renderer. The event must be of type |NSKeyDown|, |NSKeyUp|, or
// |NSFlagsChanged|.
// Returns |YES| if |event| has been handled.
- (BOOL)redispatchKeyEvent:(NSEvent*)event;

- (BOOL)performKeyEquivalent:(NSEvent*)theEvent;
@end

#endif  // ATOM_BROWSER_UI_COCOA_EVENT_PROCESSING_WINDOW_H_
