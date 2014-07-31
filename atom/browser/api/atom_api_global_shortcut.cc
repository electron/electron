// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_global_shortcut.h"

#include <string>

#include "atom/browser/ui/accelerator_util.h"
#include "atom/common/native_mate_converters/function_converter.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

using extensions::GlobalShortcutListener;

namespace atom {

namespace api {

GlobalShortcut::GlobalShortcut() {
}

GlobalShortcut::~GlobalShortcut() {
  UnregisterAll();
}

void GlobalShortcut::OnKeyPressed(const ui::Accelerator& accelerator) {
  if (accelerator_callback_map_.find(accelerator) ==
      accelerator_callback_map_.end()) {
    // This should never occur, because if it does, GlobalGlobalShortcutListener
    // notifes us with wrong accelerator.
    NOTREACHED();
    return;
  }
  accelerator_callback_map_[accelerator].Run();
}

bool GlobalShortcut::Register(const std::string& keycode,
    const base::Closure& callback) {
  ui::Accelerator accelerator;
  if (!accelerator_util::StringToAccelerator(keycode, &accelerator)) {
    LOG(ERROR) << keycode << " is invalid.";
    return false;
  }
  if (!GlobalShortcutListener::GetInstance()->RegisterAccelerator(
      accelerator, this)) {
    return false;
  }
  accelerator_callback_map_[accelerator] = callback;
  return true;
}

void GlobalShortcut::Unregister(const std::string& keycode) {
  ui::Accelerator accelerator;
  if (!accelerator_util::StringToAccelerator(keycode, &accelerator)) {
    LOG(ERROR) << "The keycode: " << keycode << " is invalid.";
    return;
  }
  if (accelerator_callback_map_.find(accelerator) ==
      accelerator_callback_map_.end()) {
    LOG(ERROR) << "The keycode: " << keycode << " isn't registered yet!";
    return;
  }
  accelerator_callback_map_.erase(accelerator);
  GlobalShortcutListener::GetInstance()->UnregisterAccelerator(
      accelerator, this);
}

void GlobalShortcut::UnregisterAll() {
  accelerator_callback_map_.clear();
  GlobalShortcutListener::GetInstance()->UnregisterAccelerators(this);
}

bool GlobalShortcut::IsRegistered(const std::string& keycode) {
  ui::Accelerator accelerator;
  if (!accelerator_util::StringToAccelerator(keycode, &accelerator)) {
    LOG(ERROR) << "The keycode: " << keycode << " is invalid.";
    return false;
  }
  return accelerator_callback_map_.find(accelerator) !=
      accelerator_callback_map_.end();
}

// static
mate::ObjectTemplateBuilder GlobalShortcut::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("register",
                 base::Bind(&GlobalShortcut::Register,
                            base::Unretained(this)))
      .SetMethod("isRegistered",
                 base::Bind(&GlobalShortcut::IsRegistered,
                            base::Unretained(this)))
      .SetMethod("unregister",
                 base::Bind(&GlobalShortcut::Unregister,
                            base::Unretained(this)))
      .SetMethod("unregisterAll",
                 base::Bind(&GlobalShortcut::UnregisterAll,
                            base::Unretained(this)));
}

// static
mate::Handle<GlobalShortcut> GlobalShortcut::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new GlobalShortcut);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("globalShortcut", atom::api::GlobalShortcut::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_global_shortcut, Initialize)
