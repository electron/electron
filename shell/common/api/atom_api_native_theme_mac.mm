// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/atom_api_native_theme.h"

#include "base/mac/sdk_forward_declarations.h"
#include "shell/browser/mac/atom_application.h"

namespace electron {

namespace api {

void NativeTheme::UpdateMacOSAppearanceForOverrideValue(
    ui::NativeTheme::OverrideShouldUseDarkColors override) {
  if (@available(macOS 10.14, *)) {
    NSAppearance* new_appearance;
    switch (override) {
      case ui::NativeTheme::OverrideShouldUseDarkColors::
          kForceDarkColorsEnabled:
        new_appearance =
            [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
        break;
      case ui::NativeTheme::OverrideShouldUseDarkColors::
          kForceDarkColorsDisabled:
        new_appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
        break;
      case ui::NativeTheme::OverrideShouldUseDarkColors::kNoOverride:
      default:
        new_appearance = nil;
        break;
    }
    [[NSApplication sharedApplication] setAppearance:new_appearance];
  }
}

}  // namespace api

}  // namespace electron
