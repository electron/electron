// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_API_ELECTRON_API_NATIVE_IMAGE_H_
#define SHELL_COMMON_API_ELECTRON_API_NATIVE_IMAGE_H_

#include <map>
#include <string>
#include <vector>

#include "base/values.h"
#include "gin/handle.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/wrappable.h"
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
class Rect;
class Size;
}  // namespace gfx

namespace gin_helper {
class Dictionary;
}

namespace electron {

namespace api {

class NativeImage : public gin_helper::Wrappable<NativeImage> {
 public:
  static gin::Handle<NativeImage> CreateEmpty(v8::Isolate* isolate);
  static gin::Handle<NativeImage> Create(v8::Isolate* isolate,
                                         const gfx::Image& image);
  static gin::Handle<NativeImage> CreateFromPNG(v8::Isolate* isolate,
                                                const char* buffer,
                                                size_t length);
  static gin::Handle<NativeImage> CreateFromJPEG(v8::Isolate* isolate,
                                                 const char* buffer,
                                                 size_t length);
  static gin::Handle<NativeImage> CreateFromPath(v8::Isolate* isolate,
                                                 const base::FilePath& path);
  static gin::Handle<NativeImage> CreateFromBitmap(
      gin_helper::ErrorThrower thrower,
      v8::Local<v8::Value> buffer,
      const gin_helper::Dictionary& options);
  static gin::Handle<NativeImage> CreateFromBuffer(
      gin_helper::ErrorThrower thrower,
      v8::Local<v8::Value> buffer,
      gin::Arguments* args);
  static gin::Handle<NativeImage> CreateFromDataURL(v8::Isolate* isolate,
                                                    const GURL& url);
  static gin::Handle<NativeImage> CreateFromNamedImage(gin::Arguments* args,
                                                       std::string name);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

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
  v8::Local<v8::Value> ToPNG(gin::Arguments* args);
  v8::Local<v8::Value> ToJPEG(v8::Isolate* isolate, int quality);
  v8::Local<v8::Value> ToBitmap(gin::Arguments* args);
  std::vector<float> GetScaleFactors();
  v8::Local<v8::Value> GetBitmap(gin::Arguments* args);
  v8::Local<v8::Value> GetNativeHandle(gin_helper::ErrorThrower thrower);
  gin::Handle<NativeImage> Resize(gin::Arguments* args,
                                  base::DictionaryValue options);
  gin::Handle<NativeImage> Crop(v8::Isolate* isolate, const gfx::Rect& rect);
  std::string ToDataURL(gin::Arguments* args);
  bool IsEmpty();
  gfx::Size GetSize(const base::Optional<float> scale_factor);
  float GetAspectRatio(const base::Optional<float> scale_factor);
  void AddRepresentation(const gin_helper::Dictionary& options);

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

}  // namespace electron

namespace gin {

// A custom converter that allows converting path to NativeImage.
template <>
struct Converter<electron::api::NativeImage*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   electron::api::NativeImage* val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::api::NativeImage** out);
};

}  // namespace gin

#endif  // SHELL_COMMON_API_ELECTRON_API_NATIVE_IMAGE_H_
