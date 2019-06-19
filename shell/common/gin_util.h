// Copyright (c) 2018 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_UTIL_H_
#define SHELL_COMMON_GIN_UTIL_H_

#include "gin/converter.h"
#include "gin/function_template.h"

namespace gin_util {

template <typename T>
bool SetMethod(v8::Local<v8::Object> recv,
               const base::StringPiece& key,
               const T& callback) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  auto context = isolate->GetCurrentContext();
  return recv
      ->Set(context, gin::StringToV8(isolate, key),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .ToChecked();
}

}  // namespace gin_util

#endif  // SHELL_COMMON_GIN_UTIL_H_
