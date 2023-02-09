// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/electron_api_image_view.h"

#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace electron::api {

ImageView::ImageView() : View(new views::ImageView()) {
  view()->set_owned_by_client();
}

ImageView::~ImageView() = default;

void ImageView::SetImage(const gfx::Image& image) {
  image_view()->SetImage(image.AsImageSkia());
}

// static
gin_helper::WrappableBase* ImageView::New(gin_helper::Arguments* args) {
  // Constructor call.
  auto* view = new ImageView();
  view->InitWithArgs(args);
  return view;
}

// static
void ImageView::BuildPrototype(v8::Isolate* isolate,
                               v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "ImageView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setImage", &ImageView::SetImage);
}

}  // namespace electron::api

namespace {

using electron::api::ImageView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("ImageView", gin_helper::CreateConstructor<ImageView>(
                            isolate, base::BindRepeating(&ImageView::New)));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_image_view, Initialize)
