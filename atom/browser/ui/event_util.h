#ifndef ATOM_BROWSER_UI_EVENT_UTIL_H_
#define ATOM_BROWSER_UI_EVENT_UTIL_H_

#import <Cocoa/Cocoa.h>
#include "ui/gfx/geometry/rect.h"

namespace event_util {

bool IsLeftButtonEvent(NSEvent* event);
bool IsRightButtonEvent(NSEvent* event);
bool IsMiddleButtonEvent(NSEvent* event);

// Retrieves a bitsum of ui::EventFlags from NSEvent.
int EventFlagsFromNSEvent(NSEvent* event);

gfx::Rect GetBoundsFromEvent(NSEvent* event);

} // namespace event_util

#endif // ATOM_BROWSER_UI_EVENT_UTIL_H_
