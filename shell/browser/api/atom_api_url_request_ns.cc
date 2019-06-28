// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_url_request_ns.h"

#include "native_mate/object_template_builder.h"

namespace electron {

namespace api {

URLRequestNS::URLRequestNS(v8::Isolate* isolate,
                           v8::Local<v8::Object> wrapper) {
  InitWith(isolate, wrapper);
}

URLRequestNS::~URLRequestNS() {}

// static
mate::WrappableBase* URLRequestNS::New(mate::Arguments* args) {
  auto* url_request = new URLRequestNS(args->isolate(), args->GetThis());
  return url_request;
}

// static
void URLRequestNS::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "URLRequest"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable();
}

}  // namespace api

}  // namespace electron
