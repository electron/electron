// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/electron_api_text_field.h"

#include "native_mate/dictionary.h"
#include "shell/common/api/constructor.h"
#include "shell/common/node_includes.h"

namespace electron {

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

}  // namespace electron

namespace {

using electron::api::TextField;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("TextField", mate::CreateConstructor<TextField>(
                            isolate, base::BindRepeating(&TextField::New)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_text_field, Initialize)
