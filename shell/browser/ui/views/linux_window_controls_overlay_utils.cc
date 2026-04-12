#include "shell/browser/ui/views/linux_window_controls_overlay_utils.h"

#include <algorithm>
#include <utility>

namespace electron::linux_window_controls_overlay {

ButtonOrder::ButtonOrder() = default;

ButtonOrder::ButtonOrder(std::vector<views::FrameButton> leading_buttons,
                         std::vector<views::FrameButton> trailing_buttons)
    : leading_buttons(std::move(leading_buttons)),
      trailing_buttons(std::move(trailing_buttons)) {}

ButtonOrder::~ButtonOrder() = default;

ButtonOrder ResolveButtonOrder(
    const std::vector<views::FrameButton>& leading_buttons,
    const std::vector<views::FrameButton>& trailing_buttons) {
  if (leading_buttons.empty() && trailing_buttons.empty()) {
    return ButtonOrder{
        {},
        {views::FrameButton::kMinimize, views::FrameButton::kMaximize,
         views::FrameButton::kClose},
    };
  }

  return ButtonOrder{leading_buttons, trailing_buttons};
}

gfx::Rect GetOverlayBounds(const gfx::Rect& client_bounds,
                           int leading_reserved_width,
                           int trailing_reserved_width,
                           int overlay_height) {
  return gfx::Rect(
      leading_reserved_width, 0,
      std::max(0, client_bounds.width() - leading_reserved_width -
                      trailing_reserved_width),
      overlay_height);
}

}  // namespace electron::linux_window_controls_overlay
