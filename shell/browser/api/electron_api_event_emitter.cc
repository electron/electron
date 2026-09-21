// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_event_emitter.h"

#include "base/check.h"
#include "base/no_destructor.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

namespace {

v8::Global<v8::Object>* GetEventEmitterPrototypeReference() {
  static base::NoDestructor<v8::Global<v8::Object>> event_emitter_prototype;
  return event_emitter_prototype.get();
}

void SetEventEmitterPrototype(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 1 || !info[0]->IsObject()) {
    isolate->ThrowException(
        v8::Exception::TypeError(v8::String::NewFromUtf8Literal(
            isolate, "Expected an EventEmitter prototype")));
    return;
  }
  GetEventEmitterPrototypeReference()->Reset(isolate, info[0].As<v8::Object>());
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  NODE_SET_METHOD(exports, "setEventEmitterPrototype",
                  &SetEventEmitterPrototype);
}

}  // namespace

namespace electron {

v8::Local<v8::Object> GetEventEmitterPrototype(v8::Isolate* isolate) {
  CHECK(!GetEventEmitterPrototypeReference()->IsEmpty());
  return GetEventEmitterPrototypeReference()->Get(isolate);
}

}  // namespace electron

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_event_emitter, Initialize)
