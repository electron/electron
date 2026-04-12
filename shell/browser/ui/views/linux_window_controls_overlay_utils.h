#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_WINDOW_CONTROLS_OVERLAY_UTILS_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_WINDOW_CONTROLS_OVERLAY_UTILS_H_

#include <vector>

#include "ui/gfx/geometry/rect.h"
#include "ui/views/window/frame_buttons.h"

namespace electron::linux_window_controls_overlay {

struct ButtonOrder {
  ButtonOrder();
  ButtonOrder(std::vector<views::FrameButton> leading_buttons,
              std::vector<views::FrameButton> trailing_buttons);
  ~ButtonOrder();

  std::vector<views::FrameButton> leading_buttons;
  std::vector<views::FrameButton> trailing_buttons;
};

ButtonOrder ResolveButtonOrder(
    const std::vector<views::FrameButton>& leading_buttons,
    const std::vector<views::FrameButton>& trailing_buttons);

gfx::Rect GetOverlayBounds(const gfx::Rect& client_bounds,
                           int leading_reserved_width,
                           int trailing_reserved_width,
                           int overlay_height);

}  // namespace electron::linux_window_controls_overlay

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_LINUX_WINDOW_CONTROLS_OVERLAY_UTILS_H_
