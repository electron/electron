// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_utility_process.h"

#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

gin::WrapperInfo UtilityProcessWrapper::kWrapperInfo = {
    gin::kEmbedderNativeGin};

UtilityProcessWrapper::UtilityProcessWrapper() = default;

UtilityProcessWrapper::~UtilityProcessWrapper() = default;

void UtilityProcessWrapper::Pin(v8::Isolate* isolate) {
  // Prevent ourselves from being GC'd until the process is exited.
  pinned_wrapper_.Reset(isolate, GetWrapper(isolate).ToLocalChecked());
}

// static
gin::Handle<UtilityProcessWrapper> UtilityProcessWrapper::Create(
    gin::Arguments* args) {
  gin_helper::Dictionary opts;
  if (!args->GetNext(&opts)) {
    args->ThrowTypeError("Expected a dictionary");
    return gin::Handle<UtilityProcessWrapper>();
  }
  auto handle = gin::CreateHandle(args->isolate(), new UtilityProcessWrapper());
  handle->Pin(args->isolate());
  return handle;
}

// static
gin::ObjectTemplateBuilder UtilityProcessWrapper::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<
      UtilityProcessWrapper>::GetObjectTemplateBuilder(isolate);
}

const char* UtilityProcessWrapper::GetTypeName() {
  return "UtilityProcessWrapper";
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("CreateProcessWrapper",
                 &electron::api::UtilityProcessWrapper::Create);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_utility_process, Initialize)
