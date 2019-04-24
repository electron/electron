// Copyright (c) 2018 Slack, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "gin/converter.h"
#include "gin/function_template.h"

namespace gin_util {

template <typename T>
bool SetMethod(v8::Local<v8::Object> recv,
               const base::StringPiece& key,
               const T& callback) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  return recv->Set(gin::StringToV8(isolate, key),
                   gin::CreateFunctionTemplate(isolate, callback)
                       ->GetFunction(isolate->GetCurrentContext())
                       .ToLocalChecked());
}

}  // namespace gin_util
