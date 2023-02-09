// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/functional/bind.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

#if !IS_MAS_BUILD()
#include "shell/common/crash_keys.h"
#endif

namespace {

v8::Local<v8::Value> GetParameters(v8::Isolate* isolate) {
  std::map<std::string, std::string> keys;
#if !IS_MAS_BUILD()
  electron::crash_keys::GetCrashKeys(&keys);
#endif
  return gin::ConvertToV8(isolate, keys);
}

#if IS_MAS_BUILD()
void SetCrashKeyStub(const std::string& key, const std::string& value) {}
void ClearCrashKeyStub(const std::string& key) {}
#endif

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
#if IS_MAS_BUILD()
  dict.SetMethod("addExtraParameter", &SetCrashKeyStub);
  dict.SetMethod("removeExtraParameter", &ClearCrashKeyStub);
#else
  dict.SetMethod("addExtraParameter", &electron::crash_keys::SetCrashKey);
  dict.SetMethod("removeExtraParameter", &electron::crash_keys::ClearCrashKey);
#endif
  dict.SetMethod("getParameters", &GetParameters);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_renderer_crash_reporter, Initialize)
