// Copyright (c) 2022 GitHub, Inc.

#include "shell/browser/api/electron_api_container_view.h"

#include "gin/handle.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_window.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

ContainerView::ContainerView(gin::Arguments* args,
                             NativeContainerView* container)
    : BaseView(args->isolate(), container), container_(container) {
  InitWithArgs(args);
}

ContainerView::~ContainerView() = default;

void ContainerView::ResetChildView(BaseView* view) {
  auto get_that_view = base_views_.find(view->GetID());
  if (get_that_view != base_views_.end()) {
    (*get_that_view).second.Reset();
    base_views_.erase(get_that_view);
  }
}

void ContainerView::ResetChildViews() {
  v8::HandleScope scope(isolate());

  for (auto& item : base_views_) {
    gin::Handle<BaseView> base_view;
    if (gin::ConvertFromV8(isolate(),
                           v8::Local<v8::Value>::New(isolate(), item.second),
                           &base_view) &&
        !base_view.IsEmpty()) {
      // There's a chance that the BaseView may have been reparented - only
      // reset if the owner view is *this* view.
      auto* parent_view = base_view->view()->GetParent();
      if (parent_view && parent_view == container_)
        base_view->view()->SetParent(nullptr);
    }

    item.second.Reset();
  }

  base_views_.clear();
}

void ContainerView::AddChildView(v8::Local<v8::Value> value) {
  gin::Handle<BaseView> base_view;
  if (value->IsObject() && gin::ConvertFromV8(isolate(), value, &base_view)) {
    auto get_that_view = base_views_.find(base_view->GetID());
    if (get_that_view == base_views_.end()) {
      if (!base_view->EnsureDetachFromParent())
        return;
      container_->AddChildView(base_view->view());
      base_views_[base_view->GetID()].Reset(isolate(), value);
    }
  }
}

void ContainerView::RemoveChildView(v8::Local<v8::Value> value) {
  gin::Handle<BaseView> base_view;
  if (value->IsObject() && gin::ConvertFromV8(isolate(), value, &base_view)) {
    auto get_that_view = base_views_.find(base_view->GetID());
    if (get_that_view != base_views_.end()) {
      container_->RemoveChildView(base_view->view());
      (*get_that_view).second.Reset(isolate(), value);
      base_views_.erase(get_that_view);
    }
  }
}

std::vector<v8::Local<v8::Value>> ContainerView::GetViews() const {
  std::vector<v8::Local<v8::Value>> ret;

  for (auto const& views_iter : base_views_) {
    if (!views_iter.second.IsEmpty())
      ret.push_back(v8::Local<v8::Value>::New(isolate(), views_iter.second));
  }

  return ret;
}

// static
gin_helper::WrappableBase* ContainerView::New(gin_helper::ErrorThrower thrower,
                                              gin::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    thrower.ThrowError("Cannot create ContainerView before app is ready");
    return nullptr;
  }

  return new ContainerView(args, new NativeContainerView());
}

// static
void ContainerView::BuildPrototype(v8::Isolate* isolate,
                                   v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "ContainerView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("addChildView", &ContainerView::AddChildView)
      .SetMethod("removeChildView", &ContainerView::RemoveChildView)
      .SetMethod("getViews", &ContainerView::GetViews)
      .Build();
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::ContainerView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("ContainerView",
           gin_helper::CreateConstructor<ContainerView>(
               isolate, base::BindRepeating(&ContainerView::New)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_container_view, Initialize)
