// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_V8_VALUE_SERIALIZER_H_
#define ELECTRON_SHELL_COMMON_V8_VALUE_SERIALIZER_H_

#include "base/containers/span.h"
#include "ui/gfx/image/image_skia_rep.h"

namespace v8 {
class Isolate;
template <class T>
class Local;
class Value;
}  // namespace v8

namespace blink {
struct CloneableMessage;
}

namespace electron {

bool SerializeV8Value(v8::Isolate* isolate,
                      v8::Local<v8::Value> value,
                      blink::CloneableMessage* out);
v8::Local<v8::Value> DeserializeV8Value(v8::Isolate* isolate,
                                        const blink::CloneableMessage& in);
v8::Local<v8::Value> DeserializeV8Value(v8::Isolate* isolate,
                                        base::span<const uint8_t> data);

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_V8_VALUE_SERIALIZER_H_
