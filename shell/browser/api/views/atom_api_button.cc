// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/atom_api_button.h"

#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

Button::Button(views::Button* impl) : View(impl) {
  view()->set_owned_by_client();
  // Make the button focusable as per the platform.
  button()->SetFocusForPlatform();
}

Button::~Button() {}

void Button::ButtonPressed(views::Button* sender, const ui::Event& event) {
  Emit("click");
}

// static
gin_helper::WrappableBase* Button::New(gin_helper::Arguments* args) {
  args->ThrowError("Button can not be created directly");
  return nullptr;
}

// static
void Button::BuildPrototype(v8::Isolate* isolate,
                            v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin_helper::StringTov8(isolate, "Button"));
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::Button;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("Button", gin_helper::CreateConstructor<Button>(
                         isolate, base::BindRepeating(&Button::New)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_button, Initialize)
