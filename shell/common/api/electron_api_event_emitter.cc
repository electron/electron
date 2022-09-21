// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_event_emitter.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "gin/per_isolate_data.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

namespace {

v8::Eternal<v8::Object>* GetEventEmitterPrototypeReference() {
  static v8::Eternal<v8::Object> event_emitter_prototype;
  return &event_emitter_prototype;
}

void SetEventEmitterPrototype(v8::Isolate* isolate,
                              v8::Local<v8::Object> proto) {
  // gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
  if (GetEventEmitterPrototypeReference()->IsEmpty()) {
    LOG(INFO) << "SetEventEmitterPrototype RESET";
    GetEventEmitterPrototypeReference()->Set(isolate, proto->Clone());
    // GetEventEmitterPrototypeReference()->ClearWeak();
  } else if (GetEventEmitterPrototypeReference()->Get(isolate)->GetCreationContextChecked() != proto->GetCreationContextChecked()) {
    LOG(INFO) << "SetEventEmitterPrototype CONTEXT DIFFERS";
    // GetEventEmitterPrototypeReference()->Reset(isolate, std::move(proto));
    // GetEventEmitterPrototypeReference()->ClearWeak();
  } else {
    LOG(INFO) << "SetEventEmitterPrototype IGNORED";
  }
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  // dict.SetMethod("setEventEmitterPrototype", &SetEventEmitterPrototype);
  dict.SetMethod("setEventEmitterPrototype", base::BindRepeating(&SetEventEmitterPrototype));
}

}  // namespace

namespace electron {

v8::Local<v8::Object> GetEventEmitterPrototype(v8::Isolate* isolate) {
  LOG(INFO) << "GetEventEmitterPrototype";
  CHECK(!GetEventEmitterPrototypeReference()->IsEmpty());
  return GetEventEmitterPrototypeReference()->Get(isolate);
}

}  // namespace electron

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_event_emitter, Initialize)
