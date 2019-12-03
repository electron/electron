// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/atom_api_label_button.h"

#include "base/strings/utf_string_conversions.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

LabelButton::LabelButton(views::LabelButton* impl) : Button(impl) {}

LabelButton::LabelButton(const std::string& text)
    : Button(new views::LabelButton(this, base::UTF8ToUTF16(text))) {}

LabelButton::~LabelButton() {}

const base::string16& LabelButton::GetText() const {
  return label_button()->GetText();
}

void LabelButton::SetText(const base::string16& text) {
  label_button()->SetText(text);
}

bool LabelButton::IsDefault() const {
  return label_button()->is_default();
}

void LabelButton::SetIsDefault(bool is_default) {
  label_button()->SetIsDefault(is_default);
}

// static
gin_helper::WrappableBase* LabelButton::New(gin_helper::Arguments* args,
                                            const std::string& text) {
  // Constructor call.
  auto* view = new LabelButton(text);
  view->InitWith(args->isolate(), args->GetThis());
  return view;
}

// static
void LabelButton::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin_helper::StringTov8(isolate, "LabelButton"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("getText", &LabelButton::GetText)
      .SetMethod("setText", &LabelButton::SetText)
      .SetMethod("isDefault", &LabelButton::IsDefault)
      .SetMethod("setIsDefault", &LabelButton::SetIsDefault);
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::LabelButton;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("LabelButton", gin_helper::CreateConstructor<LabelButton>(
                              isolate, base::BindRepeating(&LabelButton::New)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_label_button, Initialize)
