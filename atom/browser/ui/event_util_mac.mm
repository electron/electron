#include "atom/browser/ui/event_util.h"
#include "ui/base/accelerators/accelerator.h"

namespace event_util {
    bool isLeftButtonEvent(NSEvent* event) {
      NSEventType type = [event type];
      return type == NSLeftMouseDown ||
        type == NSLeftMouseDragged ||
        type == NSLeftMouseUp;
    }

    bool isRightButtonEvent(NSEvent* event) {
      NSEventType type = [event type];
      return type == NSRightMouseDown ||
        type == NSRightMouseDragged ||
        type == NSRightMouseUp;
    }

    bool isMiddleButtonEvent(NSEvent* event) {
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
      flags |= isLeftButtonEvent(event) ? ui::EF_LEFT_MOUSE_BUTTON : 0;
      flags |= isRightButtonEvent(event) ? ui::EF_RIGHT_MOUSE_BUTTON : 0;
      flags |= isMiddleButtonEvent(event) ? ui::EF_MIDDLE_MOUSE_BUTTON : 0;
      return flags;
    }

    // Retrieves a bitsum of ui::EventFlags from NSEvent.
    int EventFlagsFromNSEvent(NSEvent* event) {
      NSUInteger modifiers = [event modifierFlags];
      return EventFlagsFromNSEventWithModifiers(event, modifiers);
    }
} // namespace event_util
