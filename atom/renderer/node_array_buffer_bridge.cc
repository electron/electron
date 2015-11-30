// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/node_array_buffer_bridge.h"

#include "base/basictypes.h"
#include "atom/common/node_includes.h"
#include "native_mate/converter.h"
#include "third_party/WebKit/public/web/WebArrayBuffer.h"
#include "third_party/WebKit/public/web/WebArrayBufferConverter.h"

namespace atom {

namespace {

// global.Uint8Array;
v8::Local<v8::Function> GetUint8ArrayConstructor(
    v8::Isolate* isolate, v8::Local<v8::Context> context) {
  v8::Local<v8::Value> constructor = context->Global()->Get(
      mate::StringToV8(isolate, "Uint8Array"));
  return v8::Local<v8::Function>::Cast(constructor);
}

// new ArrayBuffer(size);
v8::Local<v8::ArrayBuffer> BlinkArrayBufferNew(
    v8::Isolate* isolate, size_t size) {
  blink::WebArrayBuffer buffer = blink::WebArrayBuffer::create(size, 1);
  return v8::Local<v8::ArrayBuffer>::Cast(
      blink::WebArrayBufferConverter::toV8Value(
          &buffer, isolate->GetCurrentContext()->Global(), isolate));
}

// new ArrayBuffer(data, size);
v8::Local<v8::ArrayBuffer> BlinkArrayBufferNewWith(
    v8::Isolate* isolate, void* data, size_t size) {
  blink::WebArrayBuffer buffer = blink::WebArrayBuffer::createExternal(
      data, size);
  return v8::Local<v8::ArrayBuffer>::Cast(
      blink::WebArrayBufferConverter::toV8Value(
          &buffer, isolate->GetCurrentContext()->Global(), isolate));
}

// new Uint8Array(array_buffer, offset, size);
v8::Local<v8::Uint8Array> BlinkUint8ArrayNew(
    v8::Local<v8::ArrayBuffer> ab, size_t offset, size_t size) {
  // Use the DOM's Uint8Array constructor to create Uint8Array.
  v8::Local<v8::Context> context = ab->CreationContext();
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Function> constructor =
      GetUint8ArrayConstructor(isolate, context);
  v8::Local<v8::Value> args[] = {
      ab, mate::ConvertToV8(isolate, offset), mate::ConvertToV8(isolate, size)
  };
  return v8::Local<v8::Uint8Array>::Cast(constructor->NewInstance(
      context, arraysize(args), args).ToLocalChecked());
}

}  // namespace

void OverrideNodeArrayBuffer() {
  node::Buffer::SetArrayBufferCreator(
      BlinkArrayBufferNew, BlinkArrayBufferNewWith, BlinkUint8ArrayNew);
}

}  // namespace atom
