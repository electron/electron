// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/javascript_environment.h"

#include <string>

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "content/public/common/content_switches.h"
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

void JavascriptEnvironment::OnMessageLoopCreated() {
  isolate_holder_.AddRunMicrotasksObserver();
}

void JavascriptEnvironment::OnMessageLoopDestroying() {
  isolate_holder_.RemoveRunMicrotasksObserver();
}

bool JavascriptEnvironment::Initialize() {
  auto cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch("debug-brk")) {
    // Need to be called before v8::Initialize().
    const char expose_debug_as[] = "--expose_debug_as=v8debug";
    v8::V8::SetFlagsFromString(expose_debug_as, sizeof(expose_debug_as) - 1);
  }

  // --js-flags.
  std::string js_flags = cmd->GetSwitchValueASCII(switches::kJavaScriptFlags);
  if (!js_flags.empty())
    v8::V8::SetFlagsFromString(js_flags.c_str(), js_flags.size());

  gin::IsolateHolder::Initialize(gin::IsolateHolder::kNonStrictMode,
                                 gin::IsolateHolder::kStableV8Extras,
                                 gin::ArrayBufferAllocator::SharedInstance());
  return true;
}

}  // namespace atom
