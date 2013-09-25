// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_EVENT_PROCESSING_WINDOW_H_
#define ATOM_BROWSER_ATOM_EVENT_PROCESSING_WINDOW_H_

#import <Cocoa/Cocoa.h>

#include "base/memory/scoped_nsobject.h"

// Override NSWindow to access unhandled keyboard events (for command
// processing); subclassing NSWindow is the only method to do
// this.
@interface AtomEventProcessingWindow : NSWindow {
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

#endif  // ATOM_BROWSER_ATOM_EVENT_PROCESSING_WINDOW_H_
