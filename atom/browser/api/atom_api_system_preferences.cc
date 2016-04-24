// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_system_preferences.h"

#include "atom/common/node_includes.h"
#include "native_mate/dictionary.h"

namespace atom {

namespace api {

SystemPreferences::SystemPreferences(v8::Isolate* isolate) {
  Init(isolate);
}

SystemPreferences::~SystemPreferences() {
}

// static
mate::Handle<SystemPreferences> SystemPreferences::Create(
    v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new SystemPreferences(isolate));
}

// static
void SystemPreferences::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("systemPreferences", atom::api::SystemPreferences::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_system_preferences, Initialize);
