// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_system_preferences.h"

#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/function_template_extensions.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

namespace electron::api {

gin::WrapperInfo SystemPreferences::kWrapperInfo = {gin::kEmbedderNativeGin};

SystemPreferences::SystemPreferences() {
#if BUILDFLAG(IS_WIN)
  InitializeWindow();
#endif
}

SystemPreferences::~SystemPreferences() {
#if BUILDFLAG(IS_WIN)
  Browser::Get()->RemoveObserver(this);
#endif
}

bool SystemPreferences::IsInvertedColorScheme() {
  return ui::NativeTheme::GetInstanceForNativeUi()
             ->GetPlatformHighContrastColorScheme() ==
         ui::NativeTheme::PlatformHighContrastColorScheme::kDark;
}

bool SystemPreferences::IsHighContrastColorScheme() {
  return ui::NativeTheme::GetInstanceForNativeUi()->UserHasContrastPreference();
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
  return gin::CreateHandle(isolate, new SystemPreferences());
}

gin::ObjectTemplateBuilder SystemPreferences::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<
             SystemPreferences>::GetObjectTemplateBuilder(isolate)
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
      .SetMethod("getColor", &SystemPreferences::GetColor)
      .SetMethod("getAccentColor", &SystemPreferences::GetAccentColor)
      .SetMethod("getMediaAccessStatus",
                 &SystemPreferences::GetMediaAccessStatus)
#endif

#if BUILDFLAG(IS_WIN)
      .SetMethod("isAeroGlassEnabled", &SystemPreferences::IsAeroGlassEnabled)
#elif BUILDFLAG(IS_MAC)
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
      .SetMethod("getEffectiveAppearance",
                 &SystemPreferences::GetEffectiveAppearance)
      .SetMethod("getAppLevelAppearance",
                 &SystemPreferences::GetAppLevelAppearance)
      .SetMethod("setAppLevelAppearance",
                 &SystemPreferences::SetAppLevelAppearance)
      .SetMethod("getSystemColor", &SystemPreferences::GetSystemColor)
      .SetMethod("canPromptTouchID", &SystemPreferences::CanPromptTouchID)
      .SetMethod("promptTouchID", &SystemPreferences::PromptTouchID)
      .SetMethod("isTrustedAccessibilityClient",
                 &SystemPreferences::IsTrustedAccessibilityClient)
      .SetMethod("askForMediaAccess", &SystemPreferences::AskForMediaAccess)
#endif
      .SetMethod("getAnimationSettings",
                 &SystemPreferences::GetAnimationSettings);
}

const char* SystemPreferences::GetTypeName() {
  return "SystemPreferences";
}

}  // namespace electron::api

namespace {

using electron::api::SystemPreferences;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("systemPreferences", SystemPreferences::Create(isolate));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_system_preferences,
                                 Initialize)
