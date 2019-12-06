// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/atom_api_resize_area.h"

#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

ResizeArea::ResizeArea() : View(new views::ResizeArea(this)) {
  view()->set_owned_by_client();
}

ResizeArea::~ResizeArea() {}

void ResizeArea::OnResize(int resize_amount, bool done_resizing) {
  Emit("resize", resize_amount, done_resizing);
}

// static
gin_helper::WrappableBase* ResizeArea::New(gin_helper::Arguments* args) {
  // Constructor call.
  auto* view = new ResizeArea();
  view->InitWith(args->isolate(), args->GetThis());
  return view;
}

// static
void ResizeArea::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin_helper::StringTov8(isolate, "ResizeArea"));
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::ResizeArea;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("ResizeArea", gin_helper::CreateConstructor<ResizeArea>(
                             isolate, base::BindRepeating(&ResizeArea::New)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_resize_area, Initialize)
