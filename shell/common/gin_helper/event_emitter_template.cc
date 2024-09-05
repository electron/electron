// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event_emitter_template.h"

#include "gin/converter.h"
#include "gin/per_isolate_data.h"
#include "shell/browser/api/electron_api_event_emitter.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-template.h"

namespace gin_helper::internal {

gin::WrapperInfo kWrapperInfo = {gin::kEmbedderNativeGin};

v8::Local<v8::FunctionTemplate> GetEventEmitterTemplate(v8::Isolate* isolate) {
  gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
  v8::Local<v8::FunctionTemplate> tmpl =
      data->GetFunctionTemplate(&kWrapperInfo);

  if (tmpl.IsEmpty()) {
    tmpl = v8::FunctionTemplate::New(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Function> func = tmpl->GetFunction(context).ToLocalChecked();

    v8::Local<v8::Object> eventemitter_prototype =
        electron::GetEventEmitterPrototype(isolate);

    v8::Local<v8::Value> func_prototype;
    CHECK(func->Get(context, gin::StringToSymbol(isolate, "prototype"))
              .ToLocal(&func_prototype));

    CHECK(func_prototype.As<v8::Object>()
              ->SetPrototypeV2(context, eventemitter_prototype)
              .ToChecked());

    data->SetFunctionTemplate(&kWrapperInfo, tmpl);
  }

  return tmpl;
}

}  // namespace gin_helper::internal
