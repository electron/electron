// Posts a click straight to a process with CGEventPostToPid(), the way
// assistive and automation software operates a window without bringing its app
// to the front. The window server does not route such an event, so the event
// has to name its window itself, which an NSEvent does.

#include "impl.h"

#import <AppKit/AppKit.h>

namespace mouse_input {

InjectResult PostClickToWindow(int pid, int window_number, int x, int y) {
  InjectResult result;
  result.expected = 2;
  const NSEventType types[] = {NSEventTypeLeftMouseDown, NSEventTypeLeftMouseUp};
  for (NSEventType type : types) {
    NSEvent* event =
        [NSEvent mouseEventWithType:type
                           location:NSMakePoint(x, y)
                      modifierFlags:0
                          timestamp:NSProcessInfo.processInfo.systemUptime
                       windowNumber:window_number
                            context:nil
                        eventNumber:0
                         clickCount:1
                           pressure:type == NSEventTypeLeftMouseDown ? 1 : 0];
    CGEventRef cg_event = event.CGEvent;
    if (!cg_event) {
      result.error = 1;
      return result;
    }
    CGEventPostToPid(pid, cg_event);
    ++result.sent;
  }
  return result;
}

}  // namespace mouse_input
