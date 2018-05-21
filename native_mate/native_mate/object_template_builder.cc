// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/object_template_builder.h"

namespace mate {

ObjectTemplateBuilder::ObjectTemplateBuilder(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ)
    : isolate_(isolate), template_(templ) {
}

ObjectTemplateBuilder::~ObjectTemplateBuilder() {
}

ObjectTemplateBuilder& ObjectTemplateBuilder::SetImpl(
    const base::StringPiece& name, v8::Local<v8::Data> val) {
  template_->Set(StringToSymbol(isolate_, name), val);
  return *this;
}

ObjectTemplateBuilder& ObjectTemplateBuilder::SetPropertyImpl(
    const base::StringPiece& name, v8::Local<v8::FunctionTemplate> getter,
    v8::Local<v8::FunctionTemplate> setter) {
#if NODE_VERSION_AT_LEAST(0, 11, 0)
  template_->SetAccessorProperty(StringToSymbol(isolate_, name), getter,
                                 setter);
#endif
  return *this;
}

ObjectTemplateBuilder& ObjectTemplateBuilder::MakeDestroyable() {
  SetMethod("destroy", base::Bind(internal::Destroyable::Destroy));
  SetMethod("isDestroyed", base::Bind(internal::Destroyable::IsDestroyed));
  return *this;
}

v8::Local<v8::ObjectTemplate> ObjectTemplateBuilder::Build() {
  v8::Local<v8::ObjectTemplate> result = template_;
  template_.Clear();
  return result;
}

}  // namespace mate
