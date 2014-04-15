// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/try_catch.h"

#include <sstream>

#include "native_mate/converter.h"

namespace mate {

TryCatch::TryCatch() {
}

TryCatch::~TryCatch() {
}

bool TryCatch::HasCaught() {
  return try_catch_.HasCaught();
}

std::string TryCatch::GetStackTrace() {
  if (!HasCaught()) {
    return "";
  }

  std::stringstream ss;
  v8::Handle<v8::Message> message = try_catch_.Message();
  ss << V8ToString(message->Get()) << std::endl
     << V8ToString(message->GetSourceLine()) << std::endl;

  v8::Handle<v8::StackTrace> trace = message->GetStackTrace();
  if (trace.IsEmpty())
    return ss.str();

  int len = trace->GetFrameCount();
  for (int i = 0; i < len; ++i) {
    v8::Handle<v8::StackFrame> frame = trace->GetFrame(i);
    ss << V8ToString(frame->GetScriptName()) << ":"
       << frame->GetLineNumber() << ":"
       << frame->GetColumn() << ": "
       << V8ToString(frame->GetFunctionName())
       << std::endl;
  }
  return ss.str();
}

}  // namespace mate
