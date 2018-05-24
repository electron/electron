// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_button.h"

#include "atom/common/api/constructor.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

Button::Button(views::Button* button) : View(button) {
  view()->set_owned_by_client();
}

Button::~Button() {}

void Button::ButtonPressed(views::Button* sender, const ui::Event& event) {
  Emit("click");
}

// static
mate::WrappableBase* Button::New(mate::Arguments* args) {
  args->ThrowError("Button can not be created directly");
  return nullptr;
}

// static
void Button::BuildPrototype(v8::Isolate* isolate,
                            v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Button"));
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::Button;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("Button",
           mate::CreateConstructor<Button>(isolate, base::Bind(&Button::New)));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_button, Initialize)
