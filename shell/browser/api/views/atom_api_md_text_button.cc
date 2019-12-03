// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/atom_api_md_text_button.h"

#include "base/strings/utf_string_conversions.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

MdTextButton::MdTextButton(const std::string& text)
    : LabelButton(views::MdTextButton::Create(this, base::UTF8ToUTF16(text))) {}

MdTextButton::~MdTextButton() {}

// static
gin_helper::WrappableBase* MdTextButton::New(gin_helper::Arguments* args,
                                             const std::string& text) {
  // Constructor call.
  auto* view = new MdTextButton(text);
  view->InitWith(args->isolate(), args->GetThis());
  return view;
}

// static
void MdTextButton::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin_helper::StringTov8(isolate, "MdTextButton"));
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::MdTextButton;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("MdTextButton",
           gin_helper::CreateConstructor<MdTextButton>(
               isolate, base::BindRepeating(&MdTextButton::New)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_md_text_button, Initialize)
