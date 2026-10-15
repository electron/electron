// Copyright (c) 2026 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_MENU_HIGHLIGHT_COLORS_H_
#define ELECTRON_SHELL_BROWSER_UI_MENU_HIGHLIGHT_COLORS_H_

#include <optional>

#include "third_party/skia/include/core/SkColor.h"

namespace ui {
class ColorProvider;
struct ColorProviderKey;
}  // namespace ui

namespace electron {

// App-supplied colors for the selected (hovered or keyboard-focused) item of
// Views menus. Unset values keep Chromium's defaults.
struct MenuHighlightColors {
  std::optional<SkColor> background;
  std::optional<SkColor> foreground;
  // Used instead of |background| / |foreground| in dark mode when set.
  std::optional<SkColor> dark_background;
  std::optional<SkColor> dark_foreground;
};

// Replaces the current colors. Menus opened afterwards use the new values.
void SetMenuHighlightColors(const MenuHighlightColors& colors);

// ColorProviderManager initializer that applies the colors passed to
// SetMenuHighlightColors(). Does nothing in high contrast or forced colors
// mode so the OS palette is preserved.
void AddMenuHighlightColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderKey& key);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_MENU_HIGHLIGHT_COLORS_H_
