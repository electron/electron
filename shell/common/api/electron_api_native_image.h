// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_API_ELECTRON_API_NATIVE_IMAGE_H_
#define ELECTRON_SHELL_COMMON_API_ELECTRON_API_NATIVE_IMAGE_H_

#include <map>
#include <string>
#include <vector>

#include "base/values.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_rep.h"

#if BUILDFLAG(IS_WIN)
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

namespace gin {
class Arguments;
}

namespace electron {

namespace api {

class NativeImage : public gin::Wrappable<NativeImage> {
 public:
  NativeImage(v8::Isolate* isolate, const gfx::Image& image);
#if BUILDFLAG(IS_WIN)
  NativeImage(v8::Isolate* isolate, const base::FilePath& hicon_path);
#endif
  ~NativeImage() override;

  // disable copy
  NativeImage(const NativeImage&) = delete;
  NativeImage& operator=(const NativeImage&) = delete;

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
#if !BUILDFLAG(IS_LINUX)
  static v8::Local<v8::Promise> CreateThumbnailFromPath(
      v8::Isolate* isolate,
      const base::FilePath& path,
      const gfx::Size& size);
#endif

  enum class OnConvertError { kThrow, kWarn };

  static bool TryConvertNativeImage(
      v8::Isolate* isolate,
      v8::Local<v8::Value> image,
      NativeImage** native_image,
      OnConvertError on_error = OnConvertError::kThrow);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

#if BUILDFLAG(IS_WIN)
  HICON GetHICON(int size);
#endif

  const gfx::Image& image() const { return image_; }

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
  gfx::Size GetSize(const absl::optional<float> scale_factor);
  float GetAspectRatio(const absl::optional<float> scale_factor);
  void AddRepresentation(const gin_helper::Dictionary& options);

  void UpdateExternalAllocatedMemoryUsage();

  // Mark the image as template image.
  void SetTemplateImage(bool setAsTemplate);
  // Determine if the image is a template image.
  bool IsTemplateImage();

#if BUILDFLAG(IS_WIN)
  base::FilePath hicon_path_;
  std::map<int, base::win::ScopedHICON> hicons_;
#endif

  gfx::Image image_;

  v8::Isolate* isolate_;
  int32_t memory_usage_ = 0;
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_API_ELECTRON_API_NATIVE_IMAGE_H_
