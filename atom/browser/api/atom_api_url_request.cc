// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_url_request.h"

namespace atom {

namespace api {
  
URLRequest::URLRequest(v8::Isolate* isolate) {
  Init(isolate);
}

URLRequest::~URLRequest() {
}

// static
mate::WrappableBase* URLRequest::New(mate::Arguments* args) {

  return new URLRequest(args->isolate());
}


// static
void URLRequest::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "WebRequest"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("start", &URLRequest::start);
}

void URLRequest::start() {

}

}  // namespace mate

}  // namepsace mate