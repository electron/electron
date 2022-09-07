// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_view.h"

#include "gin/data_object_builder.h"
#include "gin/wrappable.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/views/layout/layout_manager_base.h"

#include <limits>

namespace gin {

template <>
struct Converter<views::ChildLayout> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::ChildLayout* out) {
    gin_helper::Dictionary dict;
    if (!gin::ConvertFromV8(isolate, val, &dict))
      return false;
    gin::Handle<electron::api::View> view;
    if (!dict.Get("view", &view))
      return false;
    out->child_view = view->view();
    if (dict.Has("bounds"))
      dict.Get("bounds", &out->bounds);
    out->visible = true;
    if (dict.Has("visible"))
      dict.Get("visible", &out->visible);
    return true;
  }
};

template <>
struct Converter<views::ProposedLayout> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::ProposedLayout* out) {
    gin_helper::Dictionary dict;
    if (!gin::ConvertFromV8(isolate, val, &dict))
      return false;
    if (!dict.Get("size", &out->host_size))
      return false;
    if (!dict.Get("layouts", &out->child_layouts))
      return false;
    return true;
  }
};

template <>
struct Converter<views::SizeBound> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const views::SizeBound& in) {
    if (in.is_bounded())
      return v8::Integer::New(isolate, in.value());
    return v8::Number::New(isolate, std::numeric_limits<double>::infinity());
  }
};

template <>
struct Converter<views::SizeBounds> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const views::SizeBounds& in) {
    return gin::DataObjectBuilder(isolate)
        .Set("width", in.width())
        .Set("height", in.height())
        .Build();
  }
};
}  // namespace gin

namespace electron::api {

class JSLayoutManager : public views::LayoutManagerBase {
 public:
  JSLayoutManager(v8::Isolate* isolate, v8::Local<v8::Object> js_side)
      : js_side_(isolate, js_side) {}
  ~JSLayoutManager() override {}

  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    gin_helper::Dictionary dict(isolate, js_side_.Get(isolate));
    base::RepeatingCallback<views::ProposedLayout(
        const views::SizeBounds& size_bounds)>
        calculate_proposed_layout;
    if (dict.Get("calculateProposedLayout", &calculate_proposed_layout)) {
      views::ProposedLayout pl = calculate_proposed_layout.Run(size_bounds);
      LOG(INFO) << "Proposed Layout: " << pl.ToString();
      return pl;
    }
    return views::ProposedLayout();
  }

 private:
  v8::Global<v8::Object> js_side_;
};

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

void View::SetBounds(const gfx::Rect& bounds) {
  view()->SetBoundsRect(bounds);
}

void View::SetLayoutManager(v8::Isolate* isolate, v8::Local<v8::Object> value) {
  view_->SetLayoutManager(std::make_unique<JSLayoutManager>(isolate, value));
}

std::vector<v8::Local<v8::Value>> View::GetChildren() {
  std::vector<v8::Local<v8::Value>> ret;
  ret.reserve(child_views_.size());

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();

  for (auto& child_view : child_views_)
    ret.push_back(child_view.Get(isolate));

  return ret;
}
#endif

// static
gin_helper::WrappableBase* View::New(gin::Arguments* args) {
  View* view = new View();
  view->InitWithArgs(args);
  return view;
}

// static
v8::Local<v8::Function> View::GetConstructor(v8::Isolate* isolate) {
  static base::NoDestructor<v8::Global<v8::Function>> constructor;
  if (constructor.get()->IsEmpty()) {
    constructor->Reset(isolate, gin_helper::CreateConstructor<View>(
                                    isolate, base::BindRepeating(&View::New)));
  }
  return v8::Local<v8::Function>::New(isolate, *constructor.get());
}

// static
gin::Handle<View> View::Create(v8::Isolate* isolate) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> obj;
  if (GetConstructor(isolate)->NewInstance(context, 0, nullptr).ToLocal(&obj)) {
    gin::Handle<View> view;
    if (gin::ConvertFromV8(isolate, obj, &view))
      return view;
  }
  return gin::Handle<View>();
}

// static
void View::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "View"));
#if BUILDFLAG(ENABLE_VIEWS_API)
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("addChildView", &View::AddChildView)
      .SetMethod("addChildViewAt", &View::AddChildViewAt)
      .SetProperty("children", &View::GetChildren)
      .SetMethod("setBounds", &View::SetBounds)
      .SetMethod("setLayoutManager", &View::SetLayoutManager);
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

  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("View", View::GetConstructor(isolate));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_view, Initialize)
