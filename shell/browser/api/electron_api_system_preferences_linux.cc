// Copyright (c) 2025 Tau Gärtli <git@tau.garden>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/api/electron_api_system_preferences.h"
#include "shell/common/color_util.h"

namespace electron {

namespace api {

std::string SystemPreferences::GetAccentColor() {
  auto const color = ui_theme_->user_color();
  if (!color.has_value())
    return "";
  return ToRGBAHex(*color);
}

void SystemPreferences::OnNativeThemeUpdatedOnUI() {
  auto const new_accent_color = GetAccentColor();
  if (current_accent_color_ == new_accent_color)
    return;
  Emit("accent-color-changed", new_accent_color);
  current_accent_color_ = new_accent_color;
}

void SystemPreferences::OnNativeThemeUpdated(ui::NativeTheme* theme) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&SystemPreferences::OnNativeThemeUpdatedOnUI,
                                base::Unretained(this)));
}

}  // namespace api

}  // namespace electron
