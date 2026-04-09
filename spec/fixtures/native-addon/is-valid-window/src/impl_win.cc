#include "impl.h"

#include <windows.h>

namespace impl {

bool IsValidWindow(char* handle, size_t size) {
  if (size != sizeof(HWND))
    return false;
  HWND window = *reinterpret_cast<HWND*>(handle);
  return ::IsWindow(window);
}

}  // namespace impl
