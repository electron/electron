// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_NATIVE_IMAGE_H_
#define ATOM_COMMON_API_ATOM_API_NATIVE_IMAGE_H_

#include <map>
#include <string>

#include "native_mate/handle.h"
#include "native_mate/wrappable.h"
#include "ui/gfx/image/image.h"

#if defined(OS_WIN)
#include "base/files/file_path.h"
#include "base/win/scoped_gdi_object.h"
#endif

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

class NativeImage : public mate::Wrappable<NativeImage> {
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

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

#if defined(OS_WIN)
  HICON GetHICON(int size);
#endif

  const gfx::Image& image() const { return image_; }

 protected:
  NativeImage(v8::Isolate* isolate, const gfx::Image& image);
#if defined(OS_WIN)
  NativeImage(v8::Isolate* isolate, const base::FilePath& hicon_path);
#endif
  ~NativeImage() override;

 private:
  v8::Local<v8::Value> ToPNG(v8::Isolate* isolate);
  v8::Local<v8::Value> ToJPEG(v8::Isolate* isolate, int quality);
  v8::Local<v8::Value> GetNativeHandle(
    v8::Isolate* isolate,
    mate::Arguments* args);
  std::string ToDataURL();
  bool IsEmpty();
  gfx::Size GetSize();

  // Mark the image as template image.
  void SetTemplateImage(bool setAsTemplate);
  // Determine if the image is a template image.
  bool IsTemplateImage();

#if defined(OS_WIN)
  base::FilePath hicon_path_;
  std::map<int, base::win::ScopedHICON> hicons_;
#endif

  gfx::Image image_;

  DISALLOW_COPY_AND_ASSIGN(NativeImage);
};

}  // namespace api

}  // namespace atom

namespace mate {

// A custom converter that allows converting path to NativeImage.
template<>
struct Converter<mate::Handle<atom::api::NativeImage>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const mate::Handle<atom::api::NativeImage>& val);
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     mate::Handle<atom::api::NativeImage>* out);
};

}  // namespace mate


#endif  // ATOM_COMMON_API_ATOM_API_NATIVE_IMAGE_H_
