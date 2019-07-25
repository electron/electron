// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_web_request_ns.h"

#include "shell/browser/atom_browser_context.h"

namespace electron {

namespace api {

WebRequestNS::WebRequestNS(v8::Isolate* isolate,
                           AtomBrowserContext* browser_context) {
  Init(isolate);
  AttachAsUserData(browser_context);
}

WebRequestNS::~WebRequestNS() = default;

// static
mate::Handle<WebRequestNS> WebRequestNS::Create(
    v8::Isolate* isolate,
    AtomBrowserContext* browser_context) {
  return mate::CreateHandle(isolate,
                            new WebRequestNS(isolate, browser_context));
}

// static
void WebRequestNS::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "WebRequest"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate());
}

}  // namespace api

}  // namespace electron
