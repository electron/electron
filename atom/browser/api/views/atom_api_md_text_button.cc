// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/views/atom_api_md_text_button.h"

#include "atom/common/api/constructor.h"
#include "base/strings/utf_string_conversions.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

MdTextButton::MdTextButton(const std::string& text)
    : LabelButton(views::MdTextButton::Create(this, base::UTF8ToUTF16(text))) {}

MdTextButton::~MdTextButton() {}

// static
mate::WrappableBase* MdTextButton::New(mate::Arguments* args,
                                       const std::string& text) {
  // Constructor call.
  auto* view = new MdTextButton(text);
  view->InitWith(args->isolate(), args->GetThis());
  return view;
}

// static
void MdTextButton::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "MdTextButton"));
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::MdTextButton;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("MdTextButton", mate::CreateConstructor<MdTextButton>(
                               isolate, base::Bind(&MdTextButton::New)));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_md_text_button, Initialize)
