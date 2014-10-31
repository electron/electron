// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/ui/cocoa/event_processing_window.h"

#include "base/logging.h"
#import "content/public/browser/render_widget_host_view_mac_base.h"

@interface EventProcessingWindow ()
// Duplicate the given key event, but changing the associated window.
- (NSEvent*)keyEventForWindow:(NSWindow*)window fromKeyEvent:(NSEvent*)event;
@end

@implementation EventProcessingWindow

- (BOOL)redispatchKeyEvent:(NSEvent*)event {
  DCHECK(event);
  NSEventType eventType = [event type];
  if (eventType != NSKeyDown &&
      eventType != NSKeyUp &&
      eventType != NSFlagsChanged) {
    NOTREACHED();
    return YES;  // Pretend it's been handled in an effort to limit damage.
  }

  // Ordinarily, the event's window should be this window. However, when
  // switching between normal and fullscreen mode, we switch out the window, and
  // the event's window might be the previous window (or even an earlier one if
  // the renderer is running slowly and several mode switches occur). In this
  // rare case, we synthesize a new key event so that its associate window
  // (number) is our own.
  if ([event window] != self)
    event = [self keyEventForWindow:self fromKeyEvent:event];

  // Redispatch the event.
  eventHandled_ = YES;
  redispatchingEvent_ = YES;
  [NSApp sendEvent:event];
  redispatchingEvent_ = NO;

  // If the event was not handled by [NSApp sendEvent:], the sendEvent:
  // method below will be called, and because |redispatchingEvent_| is YES,
  // |eventHandled_| will be set to NO.
  return eventHandled_;
}

- (void)sendEvent:(NSEvent*)event {
  if (!redispatchingEvent_)
    [super sendEvent:event];
  else
    eventHandled_ = NO;
}

- (NSEvent*)keyEventForWindow:(NSWindow*)window fromKeyEvent:(NSEvent*)event {
  NSEventType eventType = [event type];

  // Convert the event's location from the original window's coordinates into
  // our own.
  NSPoint eventLoc = [event locationInWindow];
  eventLoc = [self convertRectFromScreen:
    [[event window] convertRectToScreen:NSMakeRect(eventLoc.x, eventLoc.y, 0, 0)]].origin;

  // Various things *only* apply to key down/up.
  BOOL eventIsARepeat = NO;
  NSString* eventCharacters = nil;
  NSString* eventUnmodCharacters = nil;
  if (eventType == NSKeyDown || eventType == NSKeyUp) {
    eventIsARepeat = [event isARepeat];
    eventCharacters = [event characters];
    eventUnmodCharacters = [event charactersIgnoringModifiers];
  }

  // This synthesis may be slightly imperfect: we provide nil for the context,
  // since I (viettrungluu) am sceptical that putting in the original context
  // (if one is given) is valid.
  return [NSEvent keyEventWithType:eventType
                          location:eventLoc
                     modifierFlags:[event modifierFlags]
                         timestamp:[event timestamp]
                      windowNumber:[window windowNumber]
                           context:nil
                        characters:eventCharacters
       charactersIgnoringModifiers:eventUnmodCharacters
                         isARepeat:eventIsARepeat
                           keyCode:[event keyCode]];
}


- (BOOL)performKeyEquivalent:(NSEvent*)event {
  if (redispatchingEvent_)
    return NO;

  // Give the web site a chance to handle the event. If it doesn't want to
  // handle it, it will call us back with one of the |handle*| methods above.
  NSResponder* r = [self firstResponder];
  if ([r conformsToProtocol:@protocol(RenderWidgetHostViewMacBase)])
    return [r performKeyEquivalent:event];

  if ([super performKeyEquivalent:event])
    return YES;

  return NO;
}

@end  // EventProcessingWindow
