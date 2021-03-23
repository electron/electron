#include "shell/common/platform_util.h"

#include "base/check.h"
#include "ui/aura/window.h"
#include "ui/wm/core/window_util.h"

namespace platform_util {
gfx::NativeView GetViewForWindow(gfx::NativeWindow window) {
  DCHECK(window);
  return window;
}

bool IsVisible(gfx::NativeView view) {
  return view->IsVisible();
}
}  // namespace platform_util
