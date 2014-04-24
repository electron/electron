// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include "content/public/browser/web_contents.h"
#include "native_mate/object_template_builder.h"

namespace atom {

namespace api {

WebContents::WebContents(content::WebContents* web_contents)
    : web_contents_(web_contents) {
}

WebContents::~WebContents() {
}

mate::ObjectTemplateBuilder WebContents::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate);
}

// static
mate::Handle<WebContents> WebContents::Create(
    v8::Isolate* isolate, content::WebContents* web_contents) {
  return CreateHandle(isolate, new WebContents(web_contents));
}

}  // namespace api

}  // namespace atom
