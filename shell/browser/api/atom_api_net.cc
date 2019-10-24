// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_net.h"

#include "gin/handle.h"
#include "services/network/public/cpp/features.h"
#include "shell/browser/api/atom_api_url_request.h"
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
      .SetProperty("URLRequest", &Net::URLRequest);
}

v8::Local<v8::Value> Net::URLRequest(v8::Isolate* isolate) {
  v8::Local<v8::FunctionTemplate> constructor;
  constructor = URLRequest::GetConstructor(isolate);
  return constructor->GetFunction(isolate->GetCurrentContext())
      .ToLocalChecked();
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::Net;
using electron::api::URLRequest;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();

  URLRequest::SetConstructor(isolate, base::BindRepeating(URLRequest::New));

  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("net", Net::Create(isolate));
  dict.Set("Net",
           Net::GetConstructor(isolate)->GetFunction(context).ToLocalChecked());
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_net, Initialize)
