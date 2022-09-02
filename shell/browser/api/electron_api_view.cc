// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_view.h"

#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace electron::api {

View::View(views::View* view) : view_(view) {
  view_->set_owned_by_client();
}

View::View() : View(new views::View()) {}

View::~View() {
  if (delete_view_)
    delete view_;
}

#if BUILDFLAG(ENABLE_VIEWS_API)
void View::AddChildView(gin::Handle<View> child) {
  AddChildViewAt(child, child_views_.size());
}

void View::AddChildViewAt(gin::Handle<View> child, size_t index) {
  if (index > child_views_.size())
    return;
  child_views_.emplace(child_views_.begin() + index,     // index
                       isolate(), child->GetWrapper());  // v8::Global(args...)
  view()->AddChildViewAt(child->view(), index);
}
#endif

// static
gin_helper::WrappableBase* View::New(gin::Arguments* args) {
  auto* view = new View();
  view->InitWithArgs(args);
  return view;
}

// static
void View::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "View"));
#if BUILDFLAG(ENABLE_VIEWS_API)
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("addChildView", &View::AddChildView)
      .SetMethod("addChildViewAt", &View::AddChildViewAt);
#endif
}

}  // namespace electron::api

namespace {

using electron::api::View;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  View::SetConstructor(isolate, base::BindRepeating(&View::New));

  gin_helper::Dictionary constructor(
      isolate,
      View::GetConstructor(isolate)->GetFunction(context).ToLocalChecked());

  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("View", constructor);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_view, Initialize)
