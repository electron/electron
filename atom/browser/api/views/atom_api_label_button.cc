// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/views/atom_api_label_button.h"

#include "atom/common/api/constructor.h"
#include "base/strings/utf_string_conversions.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

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
mate::WrappableBase* LabelButton::New(mate::Arguments* args,
                                      const std::string& text) {
  // Constructor call.
  auto* view = new LabelButton(text);
  view->InitWith(args->isolate(), args->GetThis());
  return view;
}

// static
void LabelButton::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "LabelButton"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("getText", &LabelButton::GetText)
      .SetMethod("setText", &LabelButton::SetText)
      .SetMethod("isDefault", &LabelButton::IsDefault)
      .SetMethod("setIsDefault", &LabelButton::SetIsDefault);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::LabelButton;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("LabelButton", mate::CreateConstructor<LabelButton>(
                              isolate, base::Bind(&LabelButton::New)));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_label_button, Initialize)
