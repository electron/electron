// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_view.h"

#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

View::View(views::View* view) : view_(view) {}

View::View() : view_(new views::View()) {
  view_->set_owned_by_client();
}

View::~View() {
  if (delete_view_)
    delete view_;
}

void View::SetLayoutManager(mate::Handle<LayoutManager> layout_manager) {
  layout_manager_.Reset(isolate(), layout_manager->GetWrapper());
  // TODO(zcbenz): New versions of Chrome takes std::unique_ptr instead of raw
  // pointer, remove the "release()" call when we upgraded to it.
  view()->SetLayoutManager(layout_manager->TakeOver().release());
}

// static
mate::WrappableBase* View::New(mate::Arguments* args) {
  auto* view = new View();
  view->InitWith(args->isolate(), args->GetThis());
  return view;
}

// static
void View::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "View"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setLayoutManager", &View::SetLayoutManager);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::View;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  View::SetConstructor(isolate, base::Bind(&View::New));

  mate::Dictionary constructor(isolate,
                               View::GetConstructor(isolate)->GetFunction());

  mate::Dictionary dict(isolate, exports);
  dict.Set("View", constructor);
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_view, Initialize)
