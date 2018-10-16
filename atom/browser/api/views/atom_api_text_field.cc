// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/views/atom_api_text_field.h"

#include "atom/common/api/constructor.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

TextField::TextField() : View(new views::Textfield()) {
  view()->set_owned_by_client();
}

TextField::~TextField() {}

void TextField::SetText(const base::string16& new_text) {
  text_field()->SetText(new_text);
}

base::string16 TextField::GetText() const {
  return text_field()->text();
}

// static
mate::WrappableBase* TextField::New(mate::Arguments* args) {
  // Constructor call.
  auto* view = new TextField();
  view->InitWith(args->isolate(), args->GetThis());
  return view;
}

// static
void TextField::BuildPrototype(v8::Isolate* isolate,
                               v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "TextField"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setText", &TextField::SetText)
      .SetMethod("getText", &TextField::GetText);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::TextField;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("TextField", mate::CreateConstructor<TextField>(
                            isolate, base::Bind(&TextField::New)));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_text_field, Initialize)
