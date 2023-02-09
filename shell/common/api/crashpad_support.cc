// Copyright (c) 2022 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

#if BUILDFLAG(IS_LINUX)
#include "components/crash/core/app/crashpad.h"  // nogncheck
#endif

namespace {

#if BUILDFLAG(IS_LINUX)
int GetCrashdumpSignalFD() {
  int fd;
  return crash_reporter::GetHandlerSocket(&fd, nullptr) ? fd : -1;
}

int GetCrashpadHandlerPID() {
  int pid;
  return crash_reporter::GetHandlerSocket(nullptr, &pid) ? pid : -1;
}
#endif

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
#if BUILDFLAG(IS_LINUX)
  dict.SetMethod("getCrashdumpSignalFD", &GetCrashdumpSignalFD);
  dict.SetMethod("getCrashpadHandlerPID", &GetCrashpadHandlerPID);
#endif
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_crashpad_support, Initialize)
