// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/javascript_environment.h"

#include "base/command_line.h"
#include "gin/array_buffer.h"
#include "gin/v8_initializer.h"

namespace atom {

JavascriptEnvironment::JavascriptEnvironment()
    : initialized_(Initialize()),
      isolate_(isolate_holder_.isolate()),
      isolate_scope_(isolate_),
      locker_(isolate_),
      handle_scope_(isolate_),
      context_(isolate_, v8::Context::New(isolate_)),
      context_scope_(v8::Local<v8::Context>::New(isolate_, context_)) {
}

bool JavascriptEnvironment::Initialize() {
  auto cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch("debug-brk")) {
    // Need to be called before v8::Initialize().
    const char expose_debug_as[] = "--expose_debug_as=v8debug";
    v8::V8::SetFlagsFromString(expose_debug_as, sizeof(expose_debug_as) - 1);
  }
  gin::IsolateHolder::Initialize(gin::IsolateHolder::kNonStrictMode,
                                 gin::ArrayBufferAllocator::SharedInstance());
  return true;
}

}  // namespace atom
