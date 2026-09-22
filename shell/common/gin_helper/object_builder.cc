// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/object_builder.h"

#include "v8/include/v8-isolate.h"

namespace gin_helper {

ObjectBuilder::ObjectBuilder(v8::Isolate* isolate)
    : isolate_{isolate},
      context_{isolate->GetCurrentContext()},
      object_{v8::Object::New(isolate)} {}

ObjectBuilder::~ObjectBuilder() = default;

v8::Local<v8::Object> ObjectBuilder::Build() {
  v8::Local<v8::Object> result = object_;
  object_.Clear();
  return result;
}

}  // namespace gin_helper
