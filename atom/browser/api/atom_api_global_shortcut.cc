// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_global_shortcut.h"

#include <string>

#include "atom/browser/api/atom_api_system_preferences.h"
#include "atom/common/native_mate_converters/accelerator_converter.h"
#include "atom/common/native_mate_converters/callback.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"
#include "atom/common/platform_util.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#endif

using extensions::GlobalShortcutListener;

namespace {

#if defined(OS_MACOSX)
bool RegisteringMediaKeyForUntrustedClient(const ui::Accelerator& accelerator) {
  if (platform_util::IsAtLeastOS10_14()) {
    constexpr ui::KeyboardCode mediaKeys[] = {
        ui::VKEY_MEDIA_PLAY_PAUSE, ui::VKEY_MEDIA_NEXT_TRACK,
        ui::VKEY_MEDIA_PREV_TRACK, ui::VKEY_MEDIA_STOP};

    if (std::find(std::begin(mediaKeys), std::end(mediaKeys),
                  accelerator.key_code()) != std::end(mediaKeys)) {
      bool trusted =
          atom::api::SystemPreferences::IsTrustedAccessibilityClient(false);
      if (!trusted)
        return true;
    }
  }
  return false;
}
#endif

}  // namespace

namespace atom {

namespace api {

GlobalShortcut::GlobalShortcut(v8::Isolate* isolate) {
  Init(isolate);
}

GlobalShortcut::~GlobalShortcut() {
  UnregisterAll();
}

void GlobalShortcut::OnKeyPressed(const ui::Accelerator& accelerator) {
  if (accelerator_callback_map_.find(accelerator) ==
      accelerator_callback_map_.end()) {
    // This should never occur, because if it does, GlobalGlobalShortcutListener
    // notifies us with wrong accelerator.
    NOTREACHED();
    return;
  }
  accelerator_callback_map_[accelerator].Run();
}

bool GlobalShortcut::Register(const ui::Accelerator& accelerator,
                              const base::Closure& callback) {
#if defined(OS_MACOSX)
  if (RegisteringMediaKeyForUntrustedClient(accelerator))
    return false;
#endif

  if (!GlobalShortcutListener::GetInstance()->RegisterAccelerator(accelerator,
                                                                  this)) {
    return false;
  }

  accelerator_callback_map_[accelerator] = callback;
  return true;
}

void GlobalShortcut::Unregister(const ui::Accelerator& accelerator) {
  if (!ContainsKey(accelerator_callback_map_, accelerator))
    return;

  accelerator_callback_map_.erase(accelerator);
  GlobalShortcutListener::GetInstance()->UnregisterAccelerator(accelerator,
                                                               this);
}

bool GlobalShortcut::IsRegistered(const ui::Accelerator& accelerator) {
  return ContainsKey(accelerator_callback_map_, accelerator);
}

void GlobalShortcut::UnregisterAll() {
  accelerator_callback_map_.clear();
  GlobalShortcutListener::GetInstance()->UnregisterAccelerators(this);
}

// static
mate::Handle<GlobalShortcut> GlobalShortcut::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new GlobalShortcut(isolate));
}

// static
void GlobalShortcut::BuildPrototype(v8::Isolate* isolate,
                                    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "GlobalShortcut"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("register", &GlobalShortcut::Register)
      .SetMethod("isRegistered", &GlobalShortcut::IsRegistered)
      .SetMethod("unregister", &GlobalShortcut::Unregister)
      .SetMethod("unregisterAll", &GlobalShortcut::UnregisterAll);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("globalShortcut", atom::api::GlobalShortcut::Create(isolate));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_global_shortcut, Initialize)
