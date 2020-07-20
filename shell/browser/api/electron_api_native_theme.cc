// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_native_theme.h"

#include <string>

#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "gin/handle.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/window_list.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

namespace electron {

namespace api {

NativeTheme::NativeTheme(v8::Isolate* isolate,
                         ui::NativeTheme* ui_theme,
                         ui::NativeTheme* web_theme)
    : ui_theme_(ui_theme), web_theme_(web_theme) {
  ui_theme_->AddObserver(this);
  Init(isolate);
}

NativeTheme::~NativeTheme() {
  ui_theme_->RemoveObserver(this);
}

void NativeTheme::OnNativeThemeUpdatedOnUI() {
  Emit("updated");
}

void NativeTheme::OnNativeThemeUpdated(ui::NativeTheme* theme) {
  base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                 base::BindOnce(&NativeTheme::OnNativeThemeUpdatedOnUI,
                                base::Unretained(this)));
}

void NativeTheme::SetThemeSource(ui::NativeTheme::ThemeSource override) {
  ui_theme_->set_theme_source(override);
  web_theme_->set_theme_source(override);
#if defined(OS_MACOSX)
  // Update the macOS appearance setting for this new override value
  UpdateMacOSAppearanceForOverrideValue(override);
#endif
#if defined(USE_X11)
  const bool dark_enabled = ShouldUseDarkColors();
  for (auto* window : WindowList::GetWindows()) {
    static_cast<NativeWindowViews*>(window)->SetGTKDarkThemeEnabled(
        dark_enabled);
  }
#endif
}

ui::NativeTheme::ThemeSource NativeTheme::GetThemeSource() const {
  return ui_theme_->theme_source();
}

bool NativeTheme::ShouldUseDarkColors() {
  return ui_theme_->ShouldUseDarkColors();
}

bool NativeTheme::ShouldUseHighContrastColors() {
  return ui_theme_->UsesHighContrastColors();
}

#if defined(OS_MACOSX)
const CFStringRef WhiteOnBlack = CFSTR("whiteOnBlack");
const CFStringRef UniversalAccessDomain = CFSTR("com.apple.universalaccess");
#endif

// TODO(MarshallOfSound): Implement for Linux
bool NativeTheme::ShouldUseInvertedColorScheme() {
#if defined(OS_MACOSX)
  CFPreferencesAppSynchronize(UniversalAccessDomain);
  Boolean keyExistsAndHasValidFormat = false;
  Boolean is_inverted = CFPreferencesGetAppBooleanValue(
      WhiteOnBlack, UniversalAccessDomain, &keyExistsAndHasValidFormat);
  if (!keyExistsAndHasValidFormat)
    return false;
  return is_inverted;
#else
  return ui_theme_->GetPlatformHighContrastColorScheme() ==
         ui::NativeTheme::PlatformHighContrastColorScheme::kDark;
#endif
}

// static
v8::Local<v8::Value> NativeTheme::Create(v8::Isolate* isolate) {
  ui::NativeTheme* ui_theme = ui::NativeTheme::GetInstanceForNativeUi();
  ui::NativeTheme* web_theme = ui::NativeTheme::GetInstanceForWeb();
  return gin::CreateHandle(isolate,
                           new NativeTheme(isolate, ui_theme, web_theme))
      .ToV8();
}

// static
void NativeTheme::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "NativeTheme"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetProperty("shouldUseDarkColors", &NativeTheme::ShouldUseDarkColors)
      .SetProperty("themeSource", &NativeTheme::GetThemeSource,
                   &NativeTheme::SetThemeSource)
      .SetProperty("shouldUseHighContrastColors",
                   &NativeTheme::ShouldUseHighContrastColors)
      .SetProperty("shouldUseInvertedColorScheme",
                   &NativeTheme::ShouldUseInvertedColorScheme);
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin::Dictionary dict(isolate, exports);
  dict.Set("nativeTheme", electron::api::NativeTheme::Create(isolate));
  dict.Set("NativeTheme", electron::api::NativeTheme::GetConstructor(isolate)
                              ->GetFunction(context)
                              .ToLocalChecked());
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

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_native_theme, Initialize)
