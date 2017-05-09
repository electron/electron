// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/javascript_environment.h"

#include <string>

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/common/content_switches.h"
#include "gin/v8_initializer.h"

#if defined(OS_WIN)
#include "atom/node/osfhandle.h"
#endif

#include "atom/common/node_includes.h"

namespace atom {

void* ArrayBufferAllocator::Allocate(size_t length) {
#if defined(OS_WIN)
  return node::ArrayBufferCalloc(length);
#else
  return calloc(1, length);
#endif
}

void* ArrayBufferAllocator::AllocateUninitialized(size_t length) {
#if defined(OS_WIN)
  return node::ArrayBufferMalloc(length);
#else
  return malloc(length);
#endif
}

void ArrayBufferAllocator::Free(void* data, size_t length) {
#if defined(OS_WIN)
  node::ArrayBufferFree(data, length);
#else
  free(data);
#endif
}

JavascriptEnvironment::JavascriptEnvironment()
    : initialized_(Initialize()),
      isolate_holder_(base::ThreadTaskRunnerHandle::Get()),
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

  // --js-flags.
  std::string js_flags = cmd->GetSwitchValueASCII(switches::kJavaScriptFlags);
  if (!js_flags.empty())
    v8::V8::SetFlagsFromString(js_flags.c_str(), js_flags.size());

  gin::IsolateHolder::Initialize(gin::IsolateHolder::kNonStrictMode,
                                 gin::IsolateHolder::kStableV8Extras,
                                 &allocator_);
  return true;
}

NodeEnvironment::NodeEnvironment(node::Environment* env) : env_(env) {
}

NodeEnvironment::~NodeEnvironment() {
  node::FreeEnvironment(env_);
}

}  // namespace atom
