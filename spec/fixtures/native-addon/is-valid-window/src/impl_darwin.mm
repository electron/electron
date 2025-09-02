#include "impl.h"

#include <Cocoa/Cocoa.h>

namespace impl {

bool IsValidWindow(char* handle, size_t size) {
  if (size != sizeof(NSView*))
    return false;
  NSView* view = *reinterpret_cast<NSView**>(handle);
  return [view isKindOfClass:[NSView class]];
}

}  // namespace impl
