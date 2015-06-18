// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_session.h"

#include "atom/browser/api/atom_api_cookies.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/callback.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

Session::Session(content::WebContents* web_contents):
  web_contents_(web_contents) {
}

Session::~Session() {
}

v8::Local<v8::Value> Session::Cookies(v8::Isolate* isolate) {
  if (cookies_.IsEmpty()) {
    auto handle = atom::api::Cookies::Create(isolate, web_contents_);
    cookies_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, cookies_);
}

mate::ObjectTemplateBuilder Session::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetProperty("cookies", &Session::Cookies);
}

// static
mate::Handle<Session> Session::Create(v8::Isolate* isolate,
                                      content::WebContents* web_contents) {
  return CreateHandle(isolate, new Session(web_contents));
}

}  // namespace api

}  // namespace atom
