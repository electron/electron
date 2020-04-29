// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "shell/common/crash_keys.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace {

void AddExtraParameter(const std::string& key, const std::string& value) {
  electron::crash_keys::SetCrashKey(key, value);
}

void RemoveExtraParameter(const std::string& key) {
  electron::crash_keys::ClearCrashKey(key);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("addExtraParameter", base::BindRepeating(&AddExtraParameter));
  dict.SetMethod("removeExtraParameter",
                 base::BindRepeating(&RemoveExtraParameter));
  /*
  dict.SetMethod("getParameters",
                 base::BindRepeating(&CrashReporter::GetParameters, reporter));
                 */
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_renderer_crash_reporter, Initialize)
