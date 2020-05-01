// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "shell/common/crash_keys.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("addExtraParameter", &electron::crash_keys::SetCrashKey);
  dict.SetMethod("removeExtraParameter", &electron::crash_keys::ClearCrashKey);
  // TODO: implement getParameters in renderer (& write a test)
  /*
  dict.SetMethod("getParameters",
                 base::BindRepeating(&CrashReporter::GetParameters, reporter));
                 */
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_renderer_crash_reporter, Initialize)
