// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/electron_api_web_utils.h"

#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/node_includes.h"
#include "third_party/blink/public/web/web_blob.h"

namespace electron::api::web_utils {

std::string GetPathForFile(v8::Isolate* isolate, v8::Local<v8::Value> file) {
  blink::WebBlob blob = blink::WebBlob::FromV8Value(isolate, file);
  if (blob.IsNull()) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "getPathForFile expected to receive a File object but one was not "
        "provided");
    return "";
  }
  return blob.Path();
}

}  // namespace electron::api::web_utils

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("getPathForFile", &electron::api::web_utils::GetPathForFile);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_renderer_web_utils, Initialize)
