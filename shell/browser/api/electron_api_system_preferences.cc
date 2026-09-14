// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_system_preferences.h"

#include "base/no_destructor.h"
#include "gin/persistent.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/animation/animation.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-cppgc.h"
#if BUILDFLAG(IS_LINUX)
#include "content/public/browser/browser_thread.h"
#include "shell/browser/api/electron_api_system_preferences.h"
#include "shell/common/color_util.h"
#endif

namespace electron::api {

gin::WrapperInfo SystemPreferences::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronSystemPreferences);

#if BUILDFLAG(IS_WIN)
SystemPreferences::SystemPreferences(v8::Isolate* isolate) {
  gin::PerIsolateData::From(isolate)->AddDisposeObserver(this);
  InitializeWindow();
}
#elif BUILDFLAG(IS_LINUX)
SystemPreferences::SystemPreferences(v8::Isolate* isolate)
    : ui_theme_(ui::NativeTheme::GetInstanceForNativeUi()) {
  gin::PerIsolateData::From(isolate)->AddDisposeObserver(this);
  ui_theme_->AddObserver(this);
}
#else
SystemPreferences::SystemPreferences(v8::Isolate* isolate) {
  gin::PerIsolateData::From(isolate)->AddDisposeObserver(this);
}
#endif

SystemPreferences::~SystemPreferences() = default;

void SystemPreferences::OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) {
  gin::PerIsolateData::From(isolate)->RemoveDisposeObserver(this);
  Dispose();
}

#if BUILDFLAG(IS_LINUX)
void SystemPreferences::Dispose() {
  ui_theme_->RemoveObserver(this);
  weak_factory_.Invalidate();
}
#elif BUILDFLAG(IS_MAC)
void SystemPreferences::Dispose() {
  ClearNotificationSubscriptions();
}
#endif

v8::Local<v8::Value> SystemPreferences::GetAnimationSettings(
    v8::Isolate* isolate) {
  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("shouldRenderRichAnimation",
           gfx::Animation::ShouldRenderRichAnimation());
  dict.Set("scrollAnimationsEnabledBySystem",
           gfx::Animation::ScrollAnimationsEnabledBySystem());
  dict.Set("prefersReducedMotion", gfx::Animation::PrefersReducedMotion());

  return dict.GetHandle();
}

#if BUILDFLAG(IS_LINUX)
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
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&SystemPreferences::OnNativeThemeUpdatedOnUI,
                     gin::WrapPersistent(weak_factory_.GetWeakCell(
                         isolate->GetCppHeap()->GetAllocationHandle()))));
}
#endif

// static
SystemPreferences* SystemPreferences::Create(v8::Isolate* isolate) {
  static base::NoDestructor<cppgc::Persistent<SystemPreferences>> instance(
      [isolate] {
        return cppgc::Persistent<SystemPreferences>(
            cppgc::MakeGarbageCollected<SystemPreferences>(
                isolate->GetCppHeap()->GetAllocationHandle(), isolate));
      }());
  return instance->Get();
}

gin::ObjectTemplateBuilder SystemPreferences::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<
             SystemPreferences>::GetObjectTemplateBuilder(isolate)
      .SetMethod("getAccentColor", &SystemPreferences::GetAccentColor)
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
      .SetMethod("getColor", &SystemPreferences::GetColor)
      .SetMethod("getMediaAccessStatus",
                 &SystemPreferences::GetMediaAccessStatus)
#endif

#if BUILDFLAG(IS_MAC)
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
      .SetMethod("getSystemColor", &SystemPreferences::GetSystemColor)
      .SetMethod("canPromptTouchID", &SystemPreferences::CanPromptTouchID)
      .SetMethod("promptTouchID", &SystemPreferences::PromptTouchID)
      .SetMethod("isTrustedAccessibilityClient",
                 &SystemPreferences::IsTrustedAccessibilityClient)
      .SetMethod("askForMediaAccess", &SystemPreferences::AskForMediaAccess)
      .SetProperty(
          "accessibilityDisplayShouldReduceTransparency",
          &SystemPreferences::AccessibilityDisplayShouldReduceTransparency)
#endif
      .SetMethod("getAnimationSettings",
                 &SystemPreferences::GetAnimationSettings);
}

const gin::WrapperInfo* SystemPreferences::wrapper_info() const {
  return &kWrapperInfo;
}

const char* SystemPreferences::GetHumanReadableName() const {
  return "Electron / SystemPreferences";
}

void SystemPreferences::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<SystemPreferences>::Trace(visitor);
#if BUILDFLAG(IS_LINUX)
  visitor->Trace(weak_factory_);
#endif
}

}  // namespace electron::api

namespace {

using electron::api::SystemPreferences;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("systemPreferences", SystemPreferences::Create(isolate));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_system_preferences,
                                  Initialize)
