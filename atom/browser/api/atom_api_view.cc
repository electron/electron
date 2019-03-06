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

#if BUILDFLAG(ENABLE_VIEW_API)
void View::SetLayoutManager(mate::Handle<LayoutManager> layout_manager) {
  layout_manager_.Reset(isolate(), layout_manager->GetWrapper());
  view()->SetLayoutManager(layout_manager->TakeOver());
}

void View::AddChildView(mate::Handle<View> child) {
  AddChildViewAt(child, child_views_.size());
}

void View::AddChildViewAt(mate::Handle<View> child, size_t index) {
  if (index > child_views_.size())
    return;
  child_views_.emplace(child_views_.begin() + index,     // index
                       isolate(), child->GetWrapper());  // v8::Global(args...)
  view()->AddChildViewAt(child->view(), index);
}
#endif

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
#if BUILDFLAG(ENABLE_VIEW_API)
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setLayoutManager", &View::SetLayoutManager)
      .SetMethod("addChildView", &View::AddChildView)
      .SetMethod("addChildViewAt", &View::AddChildViewAt);
#endif
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

  mate::Dictionary constructor(
      isolate,
      View::GetConstructor(isolate)->GetFunction(context).ToLocalChecked());

  mate::Dictionary dict(isolate, exports);
  dict.Set("View", constructor);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_view, Initialize)
