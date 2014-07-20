// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_shortcut.h"

#include <string>
#include <vector>

#include "atom/browser/ui/accelerator_util.h"
#include "base/values.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

Shortcut::Shortcut(const std::string& key) {
  is_key_valid_ = accelerator_util::StringToAccelerator(key, &accelerator_);
}

Shortcut::~Shortcut() {
  Unregister();
}

// static
mate::Wrappable* Shortcut::Create(const std::string& key) {
  return new Shortcut(key);
}

void Shortcut::OnActive() {
  Emit("active");
}

void Shortcut::OnFailed(const std::string& error_msg) {
  base::ListValue args;
  args.AppendString(error_msg);
  Emit("failed", args);
}

void Shortcut::SetKey(const std::string& key) {
  // We need to unregister the previous key before set new key eachtime.
  Unregister();
  is_key_valid_ = accelerator_util::StringToAccelerator(key, &accelerator_);
}

void Shortcut::OnKeyPressed(const ui::Accelerator& accelerator) {
  if (accelerator != accelerator_) {
    // This should never occur, because if it does, GlobalShortcutListener
    // notifes us with wrong accelerator.
    NOTREACHED();
    return;
  }

  OnActive();
}

void Shortcut::Register() {
  if (!is_key_valid_) {
    OnFailed("Shortcut is invalid.");
    return;
  }
  GlobalShortcutListener::GetInstance()->RegisterAccelerator(
      accelerator_, this);
}

void Shortcut::Unregister() {
  GlobalShortcutListener::GetInstance()->UnregisterAccelerator(accelerator_, this);
}

bool Shortcut::IsRegistered() {
  return GlobalShortcutListener::GetInstance()->IsAcceleratorRegistered(accelerator_);
}

// static
void Shortcut::BuildPrototype(v8::Isolate* isolate,
                              v8::Handle<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("setKey", &Shortcut::SetKey)
      .SetMethod("register", &Shortcut::Register)
      .SetMethod("unregister", &Shortcut::Unregister)
      .SetMethod("isRegistered", &Shortcut::IsRegistered);
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  using atom::api::Shortcut;
  v8::Isolate* isolate = context->GetIsolate();
  v8::Handle<v8::Function> constructor = mate::CreateConstructor<Shortcut>(
      isolate, "Shortcut", base::Bind(&Shortcut::Create));
  mate::Dictionary dict(isolate, exports);
  dict.Set("Shortcut", static_cast<v8::Handle<v8::Value>>(constructor));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_shortcut, Initialize)
