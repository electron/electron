// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/javascript_environment.h"

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
  gin::V8Initializer::LoadV8Snapshot();
  gin::IsolateHolder::Initialize(gin::IsolateHolder::kNonStrictMode,
                                 gin::ArrayBufferAllocator::SharedInstance());
  return true;
}

}  // namespace atom
