// Copyright (c) 2022 GitHub, Inc.

#include "shell/browser/api/electron_api_base_view.h"

#include "gin/handle.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_window.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

#if defined(TOOLKIT_VIEWS) && !BUILDFLAG(IS_MAC)
#include "ui/views/view.h"
#endif

namespace electron {

namespace api {

BaseView::BaseView(v8::Isolate* isolate, NativeView* native_view)
    : view_(native_view) {
  view_->AddObserver(this);
}

BaseView::BaseView(gin::Arguments* args, NativeView* native_view)
    : BaseView(args->isolate(), native_view) {
  InitWithArgs(args);
}

BaseView::~BaseView() {
  // Remove global reference so the JS object can be garbage collected.
  self_ref_.Reset();
}

void BaseView::InitWith(v8::Isolate* isolate, v8::Local<v8::Object> wrapper) {
  AttachAsUserData(view_.get());
  gin_helper::TrackableObject<BaseView>::InitWith(isolate, wrapper);

  // Reference this object in case it got garbage collected.
  self_ref_.Reset(isolate, wrapper);
}

void BaseView::OnChildViewDetached(NativeView* observed_view,
                                   NativeView* view) {
  auto* api_view = TrackableObject::FromWrappedClass(isolate(), view);
  if (api_view)
    ResetChildView(api_view);
}

void BaseView::OnViewIsDeleting(NativeView* observed_view) {
  RemoveFromWeakMap();
  view_->RemoveObserver(this);

  // We can not call Destroy here because we need to call Emit first, but we
  // also do not want any method to be used, so just mark as destroyed here.
  MarkDestroyed();

  EnsureDetachFromParent();
  ResetChildViews();

  // Destroy the native class when window is closed.
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, GetDestroyClosure());
}

bool BaseView::IsContainer() const {
  return view_->IsContainer();
}

void BaseView::SetBounds(const gfx::Rect& bounds) {
  view_->SetBounds(bounds);
}

gfx::Rect BaseView::GetBounds() const {
  return view_->GetBounds();
}

void BaseView::SetBackgroundColor(const std::string& color_name) {
  view_->SetBackgroundColor(ParseHexColor(color_name));
}

int32_t BaseView::GetID() const {
  return weak_map_id();
}

v8::Local<v8::Value> BaseView::GetParentView() const {
  NativeView* parent_view = view_->GetParent();
  if (parent_view) {
    auto* existing_view =
        TrackableObject::FromWrappedClass(isolate(), parent_view);
    if (existing_view)
      return existing_view->GetWrapper();
  }
  return v8::Null(isolate());
}

v8::Local<v8::Value> BaseView::GetParentWindow() const {
  NativeWindow* parent_window = view_->GetWindow();
  if (!view_->GetParent() && parent_window) {
    auto* existing_window =
        TrackableObject::FromWrappedClass(isolate(), parent_window);
    if (existing_window)
      return existing_window->GetWrapper();
  }
  return v8::Null(isolate());
}

bool BaseView::EnsureDetachFromParent() {
  auto* owner_view = view()->GetParent();
  if (owner_view) {
    owner_view->DetachChildView(view());
  } else {
    auto* owner_window = view()->GetWindow();
    if (owner_window)
      return owner_window->DetachChildView(view());
  }
  return true;
}

void BaseView::ResetChildView(BaseView* view) {}

void BaseView::ResetChildViews() {}

// static
gin_helper::WrappableBase* BaseView::New(gin_helper::ErrorThrower thrower,
                                         gin::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    thrower.ThrowError("Cannot create View before app is ready");
    return nullptr;
  }

  return new BaseView(args, new NativeView());
}

// static
void BaseView::BuildPrototype(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "BaseView"));
  gin_helper::Destroyable::MakeDestroyable(isolate, prototype);
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetProperty("id", &BaseView::GetID)
      .SetProperty("isContainer", &BaseView::IsContainer)
      .SetMethod("setBounds", &BaseView::SetBounds)
      .SetMethod("getBounds", &BaseView::GetBounds)
      .SetMethod("setBackgroundColor", &BaseView::SetBackgroundColor)
      .SetMethod("getParentView", &BaseView::GetParentView)
      .SetMethod("getParentWindow", &BaseView::GetParentWindow)
      .Build();
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::BaseView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  BaseView::SetConstructor(isolate, base::BindRepeating(&BaseView::New));

  gin_helper::Dictionary constructor(
      isolate,
      BaseView::GetConstructor(isolate)->GetFunction(context).ToLocalChecked());
  constructor.SetMethod("fromId", &BaseView::FromWeakMapID);
  constructor.SetMethod("getAllViews", &BaseView::GetAll);

  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("BaseView", constructor);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_base_view, Initialize)
