// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_event_emitter.h"

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "gin/dictionary.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

namespace {

v8::Global<v8::Object>* GetEventEmitterPrototypeReference() {
  static base::NoDestructor<v8::Global<v8::Object>> event_emitter_prototype;
  return event_emitter_prototype.get();
}

void SetEventEmitterPrototype(v8::Isolate* isolate,
                              v8::Local<v8::Object> proto) {
  GetEventEmitterPrototypeReference()->Reset(isolate, proto);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();

  gin::Dictionary dict(isolate, exports);
  dict.Set("setEventEmitterPrototype",
           base::BindRepeating(&SetEventEmitterPrototype));
}

}  // namespace

namespace electron {

v8::Local<v8::Object> GetEventEmitterPrototype(v8::Isolate* isolate) {
  CHECK(!GetEventEmitterPrototypeReference()->IsEmpty());
  return GetEventEmitterPrototypeReference()->Get(isolate);
}

}  // namespace electron

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_event_emitter, Initialize)
