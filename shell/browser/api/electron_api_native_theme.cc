// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_native_theme.h"

#include <string>

#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/native_theme/native_theme.h"

namespace electron::api {

gin::DeprecatedWrapperInfo NativeTheme::kWrapperInfo = {
    gin::kEmbedderNativeGin};

NativeTheme::NativeTheme(v8::Isolate* isolate,
                         ui::NativeTheme* ui_theme,
                         ui::NativeTheme* web_theme)
    : ui_theme_(ui_theme), web_theme_(web_theme) {
  ui_theme_->AddObserver(this);
#if BUILDFLAG(IS_WIN)
  std::ignore = hkcu_themes_regkey_.Open(HKEY_CURRENT_USER,
                                         L"Software\\Microsoft\\Windows\\"
                                         L"CurrentVersion\\Themes\\Personalize",
                                         KEY_READ);
#endif
}

NativeTheme::~NativeTheme() {
  ui_theme_->RemoveObserver(this);
}

void NativeTheme::OnNativeThemeUpdatedOnUI() {
#if BUILDFLAG(IS_WIN)
  if (hkcu_themes_regkey_.Valid()) {
    DWORD system_uses_light_theme = 1;
    hkcu_themes_regkey_.ReadValueDW(L"SystemUsesLightTheme",
                                    &system_uses_light_theme);
    bool system_dark_mode_enabled = (system_uses_light_theme == 0);
    should_use_dark_colors_for_system_integrated_ui_ =
        std::make_optional<bool>(system_dark_mode_enabled);
  }
#endif
  Emit("updated");
}

void NativeTheme::OnNativeThemeUpdated(ui::NativeTheme* theme) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&NativeTheme::OnNativeThemeUpdatedOnUI,
                                base::Unretained(this)));
}

void NativeTheme::SetThemeSource(ui::NativeTheme::ThemeSource override) {
  ui_theme_->set_theme_source(override);
  web_theme_->set_theme_source(override);
#if BUILDFLAG(IS_MAC)
  // Update the macOS appearance setting for this new override value
  UpdateMacOSAppearanceForOverrideValue(override);
#endif
  // TODO(MarshallOfSound): Update all existing browsers windows to use GTK dark
  // theme
}

ui::NativeTheme::ThemeSource NativeTheme::GetThemeSource() const {
  return ui_theme_->theme_source();
}

bool NativeTheme::ShouldUseDarkColors() {
  auto theme_source = GetThemeSource();
  if (theme_source == ui::NativeTheme::ThemeSource::kForcedLight)
    return false;
  if (theme_source == ui::NativeTheme::ThemeSource::kForcedDark)
    return true;
  return ui_theme_->preferred_color_scheme() ==
         ui::NativeTheme::PreferredColorScheme::kDark;
}

bool NativeTheme::ShouldUseHighContrastColors() {
  return ui_theme_->preferred_contrast() ==
         ui::NativeTheme::PreferredContrast::kMore;
}

bool NativeTheme::ShouldUseDarkColorsForSystemIntegratedUI() {
  return should_use_dark_colors_for_system_integrated_ui_.value_or(
      ShouldUseDarkColors());
}

bool NativeTheme::InForcedColorsMode() {
  return ui_theme_->forced_colors() !=
         ui::ColorProviderKey::ForcedColors::kNone;
}

bool NativeTheme::GetPrefersReducedTransparency() {
  return ui_theme_->prefers_reduced_transparency();
}

#if BUILDFLAG(IS_MAC)
const CFStringRef WhiteOnBlack = CFSTR("whiteOnBlack");
const CFStringRef UniversalAccessDomain = CFSTR("com.apple.universalaccess");
#endif

// TODO(MarshallOfSound): Implement for Linux
bool NativeTheme::ShouldUseInvertedColorScheme() {
#if BUILDFLAG(IS_MAC)
  CFPreferencesAppSynchronize(UniversalAccessDomain);
  Boolean keyExistsAndHasValidFormat = false;
  Boolean is_inverted = CFPreferencesGetAppBooleanValue(
      WhiteOnBlack, UniversalAccessDomain, &keyExistsAndHasValidFormat);
  if (!keyExistsAndHasValidFormat)
    return false;
  return is_inverted;
#else
  return ui_theme_->forced_colors() !=
             ui::ColorProviderKey::ForcedColors::kNone &&
         ui_theme_->preferred_color_scheme() ==
             ui::NativeTheme::PreferredColorScheme::kDark;
#endif
}

// static
gin_helper::Handle<NativeTheme> NativeTheme::Create(v8::Isolate* isolate) {
  ui::NativeTheme* ui_theme = ui::NativeTheme::GetInstanceForNativeUi();
  ui::NativeTheme* web_theme = ui::NativeTheme::GetInstanceForWeb();
  return gin_helper::CreateHandle(
      isolate, new NativeTheme(isolate, ui_theme, web_theme));
}

gin::ObjectTemplateBuilder NativeTheme::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<NativeTheme>::GetObjectTemplateBuilder(
             isolate)
      .SetProperty("shouldUseDarkColors", &NativeTheme::ShouldUseDarkColors)
      .SetProperty("themeSource", &NativeTheme::GetThemeSource,
                   &NativeTheme::SetThemeSource)
      .SetProperty("shouldUseHighContrastColors",
                   &NativeTheme::ShouldUseHighContrastColors)
      .SetProperty("shouldUseDarkColorsForSystemIntegratedUI",
                   &NativeTheme::ShouldUseDarkColorsForSystemIntegratedUI)
      .SetProperty("shouldUseInvertedColorScheme",
                   &NativeTheme::ShouldUseInvertedColorScheme)
      .SetProperty("inForcedColorsMode", &NativeTheme::InForcedColorsMode)
      .SetProperty("prefersReducedTransparency",
                   &NativeTheme::GetPrefersReducedTransparency);
}

const char* NativeTheme::GetTypeName() {
  return "NativeTheme";
}

}  // namespace electron::api

namespace {

using electron::api::NativeTheme;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin::Dictionary dict{isolate, exports};
  dict.Set("nativeTheme", NativeTheme::Create(isolate));
}

}  // namespace

namespace gin {

v8::Local<v8::Value> Converter<ui::NativeTheme::ThemeSource>::ToV8(
    v8::Isolate* isolate,
    const ui::NativeTheme::ThemeSource& val) {
  switch (val) {
    case ui::NativeTheme::ThemeSource::kForcedDark:
      return ConvertToV8(isolate, "dark");
    case ui::NativeTheme::ThemeSource::kForcedLight:
      return ConvertToV8(isolate, "light");
    case ui::NativeTheme::ThemeSource::kSystem:
    default:
      return ConvertToV8(isolate, "system");
  }
}

bool Converter<ui::NativeTheme::ThemeSource>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    ui::NativeTheme::ThemeSource* out) {
  std::string theme_source;
  if (ConvertFromV8(isolate, val, &theme_source)) {
    if (theme_source == "dark") {
      *out = ui::NativeTheme::ThemeSource::kForcedDark;
    } else if (theme_source == "light") {
      *out = ui::NativeTheme::ThemeSource::kForcedLight;
    } else if (theme_source == "system") {
      *out = ui::NativeTheme::ThemeSource::kSystem;
    } else {
      return false;
    }
    return true;
  }
  return false;
}

}  // namespace gin

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_native_theme, Initialize)
