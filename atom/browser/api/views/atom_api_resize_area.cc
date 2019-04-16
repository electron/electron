// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/views/atom_api_resize_area.h"

#include "atom/common/api/constructor.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

ResizeArea::ResizeArea() : View(new views::ResizeArea(this)) {
  view()->set_owned_by_client();
}

ResizeArea::~ResizeArea() {}

void ResizeArea::OnResize(int resize_amount, bool done_resizing) {
  Emit("resize", resize_amount, done_resizing);
}

// static
mate::WrappableBase* ResizeArea::New(mate::Arguments* args) {
  // Constructor call.
  auto* view = new ResizeArea();
  view->InitWith(args->isolate(), args->GetThis());
  return view;
}

// static
void ResizeArea::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "ResizeArea"));
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::ResizeArea;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("ResizeArea", mate::CreateConstructor<ResizeArea>(
                             isolate, base::Bind(&ResizeArea::New)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_resize_area, Initialize)
