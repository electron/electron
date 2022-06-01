// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/event_dispatching_window.h"

@implementation EventDispatchingWindow

- (void)sendEvent:(NSEvent*)event {
  if (!redispatchingEvent_)
    [super sendEvent:event];
}

- (BOOL)performKeyEquivalent:(NSEvent*)event {
  if (redispatchingEvent_)
    return NO;
  else
    return [super performKeyEquivalent:event];
}

- (void)redispatchKeyEvent:(NSEvent*)event {
  NSEventType eventType = [event type];
  if (eventType != NSEventTypeKeyDown && eventType != NSEventTypeKeyUp &&
      eventType != NSEventTypeFlagsChanged) {
    return;
  }

  // Redispatch the event.
  redispatchingEvent_ = YES;
  [NSApp sendEvent:event];
  redispatchingEvent_ = NO;
}

@end
