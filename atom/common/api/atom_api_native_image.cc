// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_native_image.h"

#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_util.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

v8::Persistent<v8::ObjectTemplate> template_;

}  // namespace

NativeImage::NativeImage(const gfx::Image& image) : image_(image) {}

NativeImage::~NativeImage() {}

mate::ObjectTemplateBuilder NativeImage::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  if (template_.IsEmpty())
    template_.Reset(isolate, mate::ObjectTemplateBuilder(isolate)
        .SetMethod("toPng", &NativeImage::ToPNG)
        .SetMethod("toJpeg", &NativeImage::ToJPEG)
        .SetMethod("isEmpty", &NativeImage::IsEmpty)
        .SetMethod("getSize", &NativeImage::GetSize)
        .Build());

  return mate::ObjectTemplateBuilder(
      isolate, v8::Local<v8::ObjectTemplate>::New(isolate, template_));
}

v8::Handle<v8::Value> NativeImage::ToPNG(v8::Isolate* isolate) {
  scoped_refptr<base::RefCountedMemory> png = image_.As1xPNGBytes();
  return node::Buffer::New(isolate,
                           reinterpret_cast<const char*>(png->front()),
                           png->size());
}

v8::Handle<v8::Value> NativeImage::ToJPEG(v8::Isolate* isolate, int quality) {
  std::vector<unsigned char> output;
  gfx::JPEG1xEncodedDataFromImage(image_, quality, &output);
  return node::Buffer::New(isolate,
                           reinterpret_cast<const char*>(&output.front()),
                           output.size());
}

bool NativeImage::IsEmpty() {
  return image_.IsEmpty();
}

gfx::Size NativeImage::GetSize() {
  return image_.Size();
}

// static
mate::Handle<NativeImage> NativeImage::Create(
    v8::Isolate* isolate, const gfx::Image& image) {
  return mate::CreateHandle(isolate, new NativeImage(image));
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromPNG(
    v8::Isolate* isolate, v8::Handle<v8::Value> buffer) {
  gfx::Image image = gfx::Image::CreateFrom1xPNGBytes(
      reinterpret_cast<unsigned char*>(node::Buffer::Data(buffer)),
      node::Buffer::Length(buffer));
  return Create(isolate, image);
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromJPEG(
    v8::Isolate* isolate, v8::Handle<v8::Value> buffer) {
  gfx::Image image = gfx::ImageFrom1xJPEGEncodedData(
      reinterpret_cast<unsigned char*>(node::Buffer::Data(buffer)),
      node::Buffer::Length(buffer));
  return Create(isolate, image);
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createFromPng", &atom::api::NativeImage::CreateFromPNG);
  dict.SetMethod("createFromJpeg", &atom::api::NativeImage::CreateFromJPEG);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_native_image, Initialize)
