// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/atom_api_box_layout.h"

#include <string>

#include "shell/browser/api/atom_api_view.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace mate {

template <>
struct Converter<views::BoxLayout::Orientation> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     views::BoxLayout::Orientation* out) {
    std::string orientation;
    if (!ConvertFromV8(isolate, val, &orientation))
      return false;
    if (orientation == "horizontal")
      *out = views::BoxLayout::Orientation::kHorizontal;
    else if (orientation == "vertical")
      *out = views::BoxLayout::Orientation::kVertical;
    else
      return false;
    return true;
  }
};

}  // namespace mate

namespace electron {

namespace api {

BoxLayout::BoxLayout(views::BoxLayout::Orientation orientation)
    : LayoutManager(new views::BoxLayout(orientation)) {}

BoxLayout::~BoxLayout() {}

void BoxLayout::SetFlexForView(gin::Handle<View> view, int flex) {
  auto* box_layout = static_cast<views::BoxLayout*>(layout_manager());
  box_layout->SetFlexForView(view->view(), flex);
}

// static
gin_helper::WrappableBase* BoxLayout::New(
    gin_helper::Arguments* args,
    views::BoxLayout::Orientation orientation) {
  auto* layout = new BoxLayout(orientation);
  layout->InitWith(args->isolate(), args->GetThis());
  return layout;
}

// static
void BoxLayout::BuildPrototype(v8::Isolate* isolate,
                               v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin_helper::StringTov8(isolate, "BoxLayout"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setFlexForView", &BoxLayout::SetFlexForView);
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::BoxLayout;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("BoxLayout", gin_helper::CreateConstructor<BoxLayout>(
                            isolate, base::BindRepeating(&BoxLayout::New)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_box_layout, Initialize)
