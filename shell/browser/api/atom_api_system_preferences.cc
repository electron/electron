// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_system_preferences.h"

#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

namespace electron {

namespace api {

SystemPreferences::SystemPreferences(v8::Isolate* isolate) {
  Init(isolate);
#if defined(OS_WIN)
  InitializeWindow();
#endif
}

SystemPreferences::~SystemPreferences() {
#if defined(OS_WIN)
  Browser::Get()->RemoveObserver(this);
#endif
}

#if !defined(OS_MACOSX)
bool SystemPreferences::IsDarkMode() {
  return ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors();
}
#endif

bool SystemPreferences::IsInvertedColorScheme() {
  return color_utils::IsInvertedColorScheme();
}

bool SystemPreferences::IsHighContrastColorScheme() {
  return ui::NativeTheme::GetInstanceForNativeUi()->UsesHighContrastColors();
}

v8::Local<v8::Value> SystemPreferences::GetAnimationSettings(
    v8::Isolate* isolate) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("shouldRenderRichAnimation",
           gfx::Animation::ShouldRenderRichAnimation());
  dict.Set("scrollAnimationsEnabledBySystem",
           gfx::Animation::ScrollAnimationsEnabledBySystem());
  dict.Set("prefersReducedMotion", gfx::Animation::PrefersReducedMotion());

  return dict.GetHandle();
}

// static
gin::Handle<SystemPreferences> SystemPreferences::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new SystemPreferences(isolate));
}

// static
void SystemPreferences::BuildPrototype(
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "SystemPreferences"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
#if defined(OS_WIN) || defined(OS_MACOSX)
      .SetMethod("getColor", &SystemPreferences::GetColor)
      .SetMethod("getAccentColor", &SystemPreferences::GetAccentColor)
#endif

#if defined(OS_WIN)
      .SetMethod("isAeroGlassEnabled", &SystemPreferences::IsAeroGlassEnabled)
#elif defined(OS_MACOSX)
      .SetMethod("postNotification", &SystemPreferences::PostNotification)
      .SetMethod("subscribeNotification",
                 &SystemPreferences::SubscribeNotification)
      .SetMethod("unsubscribeNotification",
                 &SystemPreferences::UnsubscribeNotification)
      .SetMethod("postLocalNotification",
                 &SystemPreferences::PostLocalNotification)
      .SetMethod("subscribeLocalNotification",
                 &SystemPreferences::SubscribeLocalNotification)
      .SetMethod("unsubscribeLocalNotification",
                 &SystemPreferences::UnsubscribeLocalNotification)
      .SetMethod("postWorkspaceNotification",
                 &SystemPreferences::PostWorkspaceNotification)
      .SetMethod("subscribeWorkspaceNotification",
                 &SystemPreferences::SubscribeWorkspaceNotification)
      .SetMethod("unsubscribeWorkspaceNotification",
                 &SystemPreferences::UnsubscribeWorkspaceNotification)
      .SetMethod("registerDefaults", &SystemPreferences::RegisterDefaults)
      .SetMethod("getUserDefault", &SystemPreferences::GetUserDefault)
      .SetMethod("setUserDefault", &SystemPreferences::SetUserDefault)
      .SetMethod("removeUserDefault", &SystemPreferences::RemoveUserDefault)
      .SetMethod("isSwipeTrackingFromScrollEventsEnabled",
                 &SystemPreferences::IsSwipeTrackingFromScrollEventsEnabled)
      .SetMethod("_getEffectiveAppearance",
                 &SystemPreferences::GetEffectiveAppearance)
      .SetMethod("_getAppLevelAppearance",
                 &SystemPreferences::GetAppLevelAppearance)
      .SetMethod("_setAppLevelAppearance",
                 &SystemPreferences::SetAppLevelAppearance)
      .SetProperty("appLevelAppearance",
                   &SystemPreferences::GetAppLevelAppearance,
                   &SystemPreferences::SetAppLevelAppearance)
      .SetProperty("effectiveAppearance",
                   &SystemPreferences::GetEffectiveAppearance)
      .SetMethod("getSystemColor", &SystemPreferences::GetSystemColor)
      .SetMethod("canPromptTouchID", &SystemPreferences::CanPromptTouchID)
      .SetMethod("promptTouchID", &SystemPreferences::PromptTouchID)
      .SetMethod("isTrustedAccessibilityClient",
                 &SystemPreferences::IsTrustedAccessibilityClient)
      .SetMethod("getMediaAccessStatus",
                 &SystemPreferences::GetMediaAccessStatus)
      .SetMethod("askForMediaAccess", &SystemPreferences::AskForMediaAccess)
#endif
      .SetMethod("isInvertedColorScheme",
                 &SystemPreferences::IsInvertedColorScheme)
      .SetMethod("isHighContrastColorScheme",
                 &SystemPreferences::IsHighContrastColorScheme)
      .SetMethod("isDarkMode", &SystemPreferences::IsDarkMode)
      .SetMethod("getAnimationSettings",
                 &SystemPreferences::GetAnimationSettings);
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::SystemPreferences;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("systemPreferences", SystemPreferences::Create(isolate));
  dict.Set("SystemPreferences", SystemPreferences::GetConstructor(isolate)
                                    ->GetFunction(context)
                                    .ToLocalChecked());
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_system_preferences, Initialize)
