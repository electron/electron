#ifndef ATOM_BROWSER_UI_EVENT_UTIL_H_
#define ATOM_BROWSER_UI_EVENT_UTIL_H_

#import <Cocoa/Cocoa.h>

namespace event_util {

bool isLeftButtonEvent(NSEvent* event);
bool isRightButtonEvent(NSEvent* event);
bool isMiddleButtonEvent(NSEvent* event);

// Retrieves a bitsum of ui::EventFlags from NSEvent.
int EventFlagsFromNSEvent(NSEvent* event);

} // namespace event_util

#endif // ATOM_BROWSER_UI_EVENT_UTIL_H_
