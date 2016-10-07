// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_system_preferences.h"

#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/color_utils.h"

namespace atom {

namespace api {

SystemPreferences::SystemPreferences(v8::Isolate* isolate) {
  Init(isolate);
#if defined(OS_WIN)
  InitializeWindow();
#endif
}

SystemPreferences::~SystemPreferences() {
}

#if !defined(OS_MACOSX)
bool SystemPreferences::IsDarkMode() {
  return false;
}
#endif

bool SystemPreferences::IsInvertedColorScheme() {
  return color_utils::IsInvertedColorScheme();
}

// static
mate::Handle<SystemPreferences> SystemPreferences::Create(
    v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new SystemPreferences(isolate));
}

// static
void SystemPreferences::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "SystemPreferences"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
#if defined(OS_WIN)
      .SetMethod("getAccentColor", &SystemPreferences::GetAccentColor)
      .SetMethod("isAeroGlassEnabled", &SystemPreferences::IsAeroGlassEnabled)
#elif defined(OS_MACOSX)
      .SetMethod("postNotification",
                 &SystemPreferences::PostNotification)
      .SetMethod("postLocalNotification",
                 &SystemPreferences::PostLocalNotification)
      .SetMethod("subscribeNotification",
                 &SystemPreferences::SubscribeNotification)
      .SetMethod("unsubscribeNotification",
                 &SystemPreferences::UnsubscribeNotification)
      .SetMethod("subscribeLocalNotification",
                 &SystemPreferences::SubscribeLocalNotification)
      .SetMethod("unsubscribeLocalNotification",
                 &SystemPreferences::UnsubscribeLocalNotification)
      .SetMethod("getUserDefault", &SystemPreferences::GetUserDefault)
      .SetMethod("isSwipeTrackingFromScrollEventsEnabled",
                 &SystemPreferences::IsSwipeTrackingFromScrollEventsEnabled)
#endif
      .SetMethod("isInvertedColorScheme",
                 &SystemPreferences::IsInvertedColorScheme)
      .SetMethod("isDarkMode", &SystemPreferences::IsDarkMode);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::SystemPreferences;

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("systemPreferences", SystemPreferences::Create(isolate));
  dict.Set("SystemPreferences",
           SystemPreferences::GetConstructor(isolate)->GetFunction());
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_system_preferences, Initialize);
