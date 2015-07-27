#include "atom/browser/ui/event_util.h"

#include "ui/events/event_constants.h"

namespace event_util {

    bool IsLeftButtonEvent(NSEvent* event) {
      NSEventType type = [event type];
      return type == NSLeftMouseDown ||
        type == NSLeftMouseDragged ||
        type == NSLeftMouseUp;
    }

    bool IsRightButtonEvent(NSEvent* event) {
      NSEventType type = [event type];
      return type == NSRightMouseDown ||
        type == NSRightMouseDragged ||
        type == NSRightMouseUp;
    }

    bool IsMiddleButtonEvent(NSEvent* event) {
      if ([event buttonNumber] != 2)
        return false;

      NSEventType type = [event type];
      return type == NSOtherMouseDown ||
        type == NSOtherMouseDragged ||
        type == NSOtherMouseUp;
    }

    int EventFlagsFromNSEventWithModifiers(NSEvent* event, NSUInteger modifiers) {
      int flags = 0;
      flags |= (modifiers & NSAlphaShiftKeyMask) ? ui::EF_CAPS_LOCK_DOWN : 0;
      flags |= (modifiers & NSShiftKeyMask) ? ui::EF_SHIFT_DOWN : 0;
      flags |= (modifiers & NSControlKeyMask) ? ui::EF_CONTROL_DOWN : 0;
      flags |= (modifiers & NSAlternateKeyMask) ? ui::EF_ALT_DOWN : 0;
      flags |= (modifiers & NSCommandKeyMask) ? ui::EF_COMMAND_DOWN : 0;
      flags |= IsLeftButtonEvent(event) ? ui::EF_LEFT_MOUSE_BUTTON : 0;
      flags |= IsRightButtonEvent(event) ? ui::EF_RIGHT_MOUSE_BUTTON : 0;
      flags |= IsMiddleButtonEvent(event) ? ui::EF_MIDDLE_MOUSE_BUTTON : 0;
      return flags;
    }

    // Retrieves a bitsum of ui::EventFlags from NSEvent.
    int EventFlagsFromNSEvent(NSEvent* event) {
      NSUInteger modifiers = [event modifierFlags];
      return EventFlagsFromNSEventWithModifiers(event, modifiers);
    }

    gfx::Rect GetBoundsFromEvent(NSEvent* event) {
        NSRect frame = event.window.frame;

        gfx::Rect bounds(
            frame.origin.x,
            0,
            NSWidth(frame),
            NSHeight(frame)
        );

        NSScreen* screen = [[NSScreen screens] objectAtIndex:0];
        bounds.set_y(NSHeight([screen frame]) - NSMaxY(frame));
        return bounds;
    }

} // namespace event_util
