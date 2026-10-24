// Copyright (c) 2026 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/menu_highlight_colors.h"

#include "ui/color/color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_provider_manager.h"
#include "ui/color/color_recipe.h"

namespace electron {

namespace {

MenuHighlightColors& GetMenuHighlightColors() {
  static MenuHighlightColors colors;
  return colors;
}

}  // namespace

void SetMenuHighlightColors(const MenuHighlightColors& colors) {
  GetMenuHighlightColors() = colors;
  // Widgets fetch their ColorProvider from the manager on demand, so dropping
  // the cache is enough for newly opened menus to pick up the change.
  ui::ColorProviderManager::Get().ResetColorProviderCache();
}

void AddMenuHighlightColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderKey& key) {
  if (key.contrast_mode == ui::ColorProviderKey::ContrastMode::kHigh ||
      key.forced_colors != ui::ColorProviderKey::ForcedColors::kNone) {
    return;
  }

  const MenuHighlightColors& colors = GetMenuHighlightColors();
  const bool dark = key.color_mode == ui::ColorProviderKey::ColorMode::kDark;
  const std::optional<SkColor> background =
      dark && colors.dark_background ? colors.dark_background
                                     : colors.background;
  const std::optional<SkColor> foreground =
      dark && colors.dark_foreground ? colors.dark_foreground
                                     : colors.foreground;
  if (!background && !foreground)
    return;

  ui::ColorMixer& mixer = provider->AddMixer();
  if (background)
    mixer[ui::kColorMenuItemBackgroundSelected] = {*background};
  if (foreground)
    mixer[ui::kColorMenuItemForegroundSelected] = {*foreground};
}

}  // namespace electron
