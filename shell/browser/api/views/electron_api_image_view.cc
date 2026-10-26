// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/electron_api_image_view.h"

#include <memory>

#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/image/image.h"
#include "ui/views/controls/image_view.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"

namespace electron::api {

const gin::WrapperInfo ImageView::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronView);

ImageView::ImageView() : View(std::make_unique<views::ImageView>()) {}

ImageView::~ImageView() = default;

views::ImageView* ImageView::image_view() const {
  return static_cast<views::ImageView*>(view());
}

void ImageView::SetImage(const gfx::Image& image) {
  if (views::ImageView* image_view = this->image_view())
    image_view->SetImage(ui::ImageModel::FromImage(image));
}

// static
ImageView* ImageView::New(gin::Arguments* const args) {
  if (!gin_helper::ThrowIfNotConstructCall(args))
    return nullptr;
  auto* view = cppgc::MakeGarbageCollected<ImageView>(
      args->isolate()->GetCppHeap()->GetAllocationHandle());
  gin_helper::BindToConstructCall(args, view);
  return view;
}

const gin::WrapperInfo* ImageView::wrapper_info() const {
  return &kWrapperInfo;
}

const char* ImageView::GetHumanReadableName() const {
  return "Electron / ImageView";
}

// static
void ImageView::FillObjectTemplate(v8::Isolate* isolate,
                                   v8::Local<v8::ObjectTemplate> templ) {
  gin_helper::ObjectTemplateBuilder(isolate, templ)
      .SetMethod<&ImageView::SetImage>("setImage");
}

}  // namespace electron::api

namespace {

using electron::api::ImageView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("ImageView", gin_helper::Constructible<ImageView>::GetConstructor(
                            isolate, context));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_image_view, Initialize)
