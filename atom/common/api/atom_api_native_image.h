// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_NATIVE_IMAGE_H_
#define ATOM_COMMON_API_ATOM_API_NATIVE_IMAGE_H_

#include <string>

#include "native_mate/handle.h"
#include "native_mate/wrappable.h"
#include "ui/gfx/image/image.h"

class GURL;

namespace base {
class FilePath;
}

namespace gfx {
class Size;
}

namespace mate {
class Arguments;
}

namespace atom {

namespace api {

class NativeImage : public mate::Wrappable {
 public:
  static mate::Handle<NativeImage> CreateEmpty(v8::Isolate* isolate);
  static mate::Handle<NativeImage> Create(
      v8::Isolate* isolate, const gfx::Image& image);
  static mate::Handle<NativeImage> CreateFromPNG(
      v8::Isolate* isolate, const char* buffer, size_t length);
  static mate::Handle<NativeImage> CreateFromJPEG(
      v8::Isolate* isolate, const char* buffer, size_t length);
  static mate::Handle<NativeImage> CreateFromPath(
      v8::Isolate* isolate, const base::FilePath& path);
  static mate::Handle<NativeImage> CreateFromBuffer(
      mate::Arguments* args, v8::Local<v8::Value> buffer);
  static mate::Handle<NativeImage> CreateFromDataURL(
      v8::Isolate* isolate, const GURL& url);

  // The default constructor should only be used by image_converter.cc.
  NativeImage();

  const gfx::Image& image() const { return image_; }

 protected:
  explicit NativeImage(const gfx::Image& image);
  virtual ~NativeImage();

  // mate::Wrappable:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  v8::Local<v8::Value> ToPNG(v8::Isolate* isolate);
  v8::Local<v8::Value> ToJPEG(v8::Isolate* isolate, int quality);
  std::string ToDataURL();
  bool IsEmpty();
  gfx::Size GetSize();

  // Mark the image as template image.
  void SetTemplateImage(bool setAsTemplate);
  // Determine if the image is a template image.
  bool IsTemplateImage();

  gfx::Image image_;

  DISALLOW_COPY_AND_ASSIGN(NativeImage);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_NATIVE_IMAGE_H_
