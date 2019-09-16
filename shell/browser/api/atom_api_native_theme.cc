// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_native_theme.h"

#include <string>

#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

namespace electron {

namespace api {

NativeTheme::NativeTheme(v8::Isolate* isolate, ui::NativeTheme* theme)
    : theme_(theme) {
  theme_->AddObserver(this);
  Init(isolate);
}

NativeTheme::~NativeTheme() {
  theme_->RemoveObserver(this);
}

void NativeTheme::OnNativeThemeUpdatedOnUI() {
  Emit("updated");
}

void NativeTheme::OnNativeThemeUpdated(ui::NativeTheme* theme) {
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&NativeTheme::OnNativeThemeUpdatedOnUI,
                     base::Unretained(this)));
}

void NativeTheme::SetThemeSource(ui::NativeTheme::ThemeSource override) {
  theme_->set_theme_source(override);
#if defined(OS_MACOSX)
  // Update the macOS appearance setting for this new override value
  UpdateMacOSAppearanceForOverrideValue(override);
#endif
  // TODO(MarshallOfSound): Update all existing browsers windows to use GTK dark
  // theme
}

ui::NativeTheme::ThemeSource NativeTheme::GetThemeSource() const {
  return theme_->theme_source();
}

bool NativeTheme::ShouldUseDarkColors() {
  return theme_->ShouldUseDarkColors();
}

bool NativeTheme::ShouldUseHighContrastColors() {
  return theme_->UsesHighContrastColors();
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
  return color_utils::IsInvertedColorScheme();
#endif
}

// static
v8::Local<v8::Value> NativeTheme::Create(v8::Isolate* isolate) {
  ui::NativeTheme* theme = ui::NativeTheme::GetInstanceForNativeUi();
  return mate::CreateHandle(isolate, new NativeTheme(isolate, theme)).ToV8();
}

// static
void NativeTheme::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "NativeTheme"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
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
  mate::Dictionary dict(isolate, exports);
  dict.Set("nativeTheme", electron::api::NativeTheme::Create(isolate));
  dict.Set("NativeTheme", electron::api::NativeTheme::GetConstructor(isolate)
                              ->GetFunction(context)
                              .ToLocalChecked());
}

}  // namespace

namespace mate {

v8::Local<v8::Value> Converter<ui::NativeTheme::ThemeSource>::ToV8(
    v8::Isolate* isolate,
    const ui::NativeTheme::ThemeSource& val) {
  switch (val) {
    case ui::NativeTheme::ThemeSource::kForcedDark:
      return mate::ConvertToV8(isolate, "dark");
    case ui::NativeTheme::ThemeSource::kForcedLight:
      return mate::ConvertToV8(isolate, "light");
    case ui::NativeTheme::ThemeSource::kSystem:
    default:
      return mate::ConvertToV8(isolate, "system");
  }
}

bool Converter<ui::NativeTheme::ThemeSource>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    ui::NativeTheme::ThemeSource* out) {
  std::string theme_source;
  if (mate::ConvertFromV8(isolate, val, &theme_source)) {
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

}  // namespace mate

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_common_native_theme, Initialize)
