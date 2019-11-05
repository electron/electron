// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_global_shortcut.h"

#include <string>
#include <vector>

#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/api/atom_api_system_preferences.h"
#include "shell/common/gin_converters/accelerator_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#endif

using extensions::GlobalShortcutListener;

namespace {

#if defined(OS_MACOSX)
bool RegisteringMediaKeyForUntrustedClient(const ui::Accelerator& accelerator) {
  if (base::mac::IsAtLeastOS10_14()) {
    constexpr ui::KeyboardCode mediaKeys[] = {
        ui::VKEY_MEDIA_PLAY_PAUSE, ui::VKEY_MEDIA_NEXT_TRACK,
        ui::VKEY_MEDIA_PREV_TRACK, ui::VKEY_MEDIA_STOP};

    if (std::find(std::begin(mediaKeys), std::end(mediaKeys),
                  accelerator.key_code()) != std::end(mediaKeys)) {
      bool trusted =
          electron::api::SystemPreferences::IsTrustedAccessibilityClient(false);
      if (!trusted)
        return true;
    }
  }
  return false;
}
#endif

}  // namespace

namespace electron {

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

bool GlobalShortcut::RegisterAll(
    const std::vector<ui::Accelerator>& accelerators,
    const base::Closure& callback) {
  std::vector<ui::Accelerator> registered;

  for (auto& accelerator : accelerators) {
    if (!Register(accelerator, callback)) {
      // unregister all shortcuts if any failed
      UnregisterSome(registered);
      return false;
    }

    registered.push_back(accelerator);
  }
  return true;
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
  if (accelerator_callback_map_.erase(accelerator) == 0)
    return;

  GlobalShortcutListener::GetInstance()->UnregisterAccelerator(accelerator,
                                                               this);
}

void GlobalShortcut::UnregisterSome(
    const std::vector<ui::Accelerator>& accelerators) {
  for (auto& accelerator : accelerators) {
    Unregister(accelerator);
  }
}

bool GlobalShortcut::IsRegistered(const ui::Accelerator& accelerator) {
  return base::Contains(accelerator_callback_map_, accelerator);
}

void GlobalShortcut::UnregisterAll() {
  accelerator_callback_map_.clear();
  GlobalShortcutListener::GetInstance()->UnregisterAccelerators(this);
}

// static
gin::Handle<GlobalShortcut> GlobalShortcut::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new GlobalShortcut(isolate));
}

// static
void GlobalShortcut::BuildPrototype(v8::Isolate* isolate,
                                    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "GlobalShortcut"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("registerAll", &GlobalShortcut::RegisterAll)
      .SetMethod("register", &GlobalShortcut::Register)
      .SetMethod("isRegistered", &GlobalShortcut::IsRegistered)
      .SetMethod("unregister", &GlobalShortcut::Unregister)
      .SetMethod("unregisterAll", &GlobalShortcut::UnregisterAll);
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("globalShortcut", electron::api::GlobalShortcut::Create(isolate));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_global_shortcut, Initialize)
