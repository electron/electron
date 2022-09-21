// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event_emitter_mixin.h"

#include "base/logging.h"
#include "gin/public/wrapper_info.h"

namespace {

v8::Eternal<v8::Object>* GetEventEmitterPrototypeReference() {
  static v8::Eternal<v8::Object> event_emitter_prototype;
  return &event_emitter_prototype;
}

v8::Local<v8::Object> GetEventEmitterPrototype(v8::Isolate* isolate) {
  CHECK(!GetEventEmitterPrototypeReference()->IsEmpty());
  return GetEventEmitterPrototypeReference()->Get(isolate);
}

}  // namespace

namespace gin_helper::internal {

gin::WrapperInfo kWrapperInfo = {gin::kEmbedderNativeGin};

void SetEventEmitterPrototype(v8::Isolate* isolate,
                              v8::Local<v8::Object> proto) {
  if (GetEventEmitterPrototypeReference()->IsEmpty()) {
    GetEventEmitterPrototypeReference()->Set(isolate, proto->Clone());
  }

  // Ensure whichever context is setting the prototype also applies it to all
  // APIs using the EventEmitter template.
  UpdateEventEmitterTemplatePrototype(isolate);
}

// Templates create a unique function per context. Calling this method will set
// the template's function prototype to that of EventEmitter.
void UpdateEventEmitterTemplatePrototype(v8::Isolate* isolate) {
  gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
  v8::Local<v8::FunctionTemplate> tmpl =
      data->GetFunctionTemplate(&kWrapperInfo);

  if (!tmpl.IsEmpty()) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Function> func = tmpl->GetFunction(context).ToLocalChecked();

    v8::Local<v8::Object> eventemitter_prototype =
        GetEventEmitterPrototype(isolate);

    v8::Local<v8::Value> func_prototype;
    CHECK(func->Get(context, gin::StringToSymbol(isolate, "prototype"))
              .ToLocal(&func_prototype));

    CHECK(func_prototype.As<v8::Object>()
              ->SetPrototype(context, eventemitter_prototype)
              .ToChecked());
  }
}

v8::Local<v8::FunctionTemplate> GetEventEmitterTemplate(v8::Isolate* isolate) {
  gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
  v8::Local<v8::FunctionTemplate> tmpl =
      data->GetFunctionTemplate(&kWrapperInfo);

  if (tmpl.IsEmpty()) {
    tmpl = v8::FunctionTemplate::New(isolate);
    data->SetFunctionTemplate(&kWrapperInfo, tmpl);

    // The EventEmitter template is created once per isolate. However, setting
    // the EventEmitter prototype is required once per context. Updating the
    // prototype here covers the case for the main process which only ever has
    // a single context. Using this in the renderer requires calls for each
    // context.
    UpdateEventEmitterTemplatePrototype(isolate);
  }

  return tmpl;
}

}  // namespace gin_helper::internal
