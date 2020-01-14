// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_net.h"

#include <string>

#include "gin/handle.h"
#include "services/network/public/cpp/features.h"
#include "shell/browser/api/atom_api_url_loader.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"

#include "shell/common/node_includes.h"

namespace electron {

namespace api {

Net::Net(v8::Isolate* isolate) {
  Init(isolate);
}

Net::~Net() = default;

// static
v8::Local<v8::Value> Net::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new Net(isolate)).ToV8();
}

// static
void Net::BuildPrototype(v8::Isolate* isolate,
                         v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "Net"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetProperty("URLLoader", &Net::URLLoader);
}

v8::Local<v8::Value> Net::URLLoader(v8::Isolate* isolate) {
  v8::Local<v8::FunctionTemplate> constructor;
  constructor = SimpleURLLoaderWrapper::GetConstructor(isolate);
  return constructor->GetFunction(isolate->GetCurrentContext())
      .ToLocalChecked();
}

}  // namespace api

}  // namespace electron

namespace {

bool IsValidHeaderName(std::string header_name) {
  return net::HttpUtil::IsValidHeaderName(header_name);
}

bool IsValidHeaderValue(std::string header_value) {
  return net::HttpUtil::IsValidHeaderValue(header_value);
}

using electron::api::Net;
using electron::api::SimpleURLLoaderWrapper;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();

  SimpleURLLoaderWrapper::SetConstructor(
      isolate, base::BindRepeating(SimpleURLLoaderWrapper::New));

  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("net", Net::Create(isolate));
  dict.Set("Net",
           Net::GetConstructor(isolate)->GetFunction(context).ToLocalChecked());
  dict.SetMethod("_isValidHeaderName", &IsValidHeaderName);
  dict.SetMethod("_isValidHeaderValue", &IsValidHeaderValue);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_net, Initialize)
