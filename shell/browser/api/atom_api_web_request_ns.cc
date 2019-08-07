// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_web_request_ns.h"

#include "gin/object_template_builder.h"
#include "shell/browser/atom_browser_context.h"

namespace electron {

namespace api {

gin::WrapperInfo WebRequestNS::kWrapperInfo = {gin::kEmbedderNativeGin};

WebRequestNS::WebRequestNS(v8::Isolate* isolate,
                           AtomBrowserContext* browser_context) {}

WebRequestNS::~WebRequestNS() = default;

gin::ObjectTemplateBuilder WebRequestNS::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<WebRequestNS>::GetObjectTemplateBuilder(isolate);
}

const char* WebRequestNS::GetTypeName() {
  return "WebRequest";
}

// static
gin::Handle<WebRequestNS> WebRequestNS::Create(
    v8::Isolate* isolate,
    AtomBrowserContext* browser_context) {
  return gin::CreateHandle(isolate, new WebRequestNS(isolate, browser_context));
}

}  // namespace api

}  // namespace electron
