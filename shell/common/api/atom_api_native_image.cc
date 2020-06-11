// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/atom_api_native_image.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/strings/pattern.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "native_mate/object_template_builder.h"
#include "net/base/data_url.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/native_mate_converters/file_path_converter.h"
#include "shell/common/native_mate_converters/gfx_converter.h"
#include "shell/common/native_mate_converters/gurl_converter.h"
#include "shell/common/native_mate_converters/value_converter.h"
#include "shell/common/node_includes.h"
#include "shell/common/skia_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPixelRef.h"
#include "ui/base/layout.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/image/image_util.h"

#if defined(OS_WIN)
#include "base/win/scoped_gdi_object.h"
#include "shell/common/asar/archive.h"
#include "ui/gfx/icon_util.h"
#endif

namespace electron {

namespace api {

namespace {

// Get the scale factor from options object at the first argument
float GetScaleFactorFromOptions(mate::Arguments* args) {
  float scale_factor = 1.0f;
  mate::Dictionary options;
  if (args->GetNext(&options))
    options.Get("scaleFactor", &scale_factor);
  return scale_factor;
}

base::FilePath NormalizePath(const base::FilePath& path) {
  if (!path.ReferencesParent()) {
    return path;
  }

  base::FilePath absolute_path = MakeAbsoluteFilePath(path);
  // MakeAbsoluteFilePath returns an empty path on failures so use original path
  if (absolute_path.empty()) {
    return path;
  } else {
    return absolute_path;
  }
}

#if defined(OS_MACOSX)
bool IsTemplateFilename(const base::FilePath& path) {
  return (base::MatchPattern(path.value(), "*Template.*") ||
          base::MatchPattern(path.value(), "*Template@*x.*"));
}
#endif

#if defined(OS_WIN)
base::win::ScopedHICON ReadICOFromPath(int size, const base::FilePath& path) {
  // If file is in asar archive, we extract it to a temp file so LoadImage can
  // load it.
  base::FilePath asar_path, relative_path;
  base::FilePath image_path(path);
  if (asar::GetAsarArchivePath(image_path, &asar_path, &relative_path)) {
    std::shared_ptr<asar::Archive> archive =
        asar::GetOrCreateAsarArchive(asar_path);
    if (archive)
      archive->CopyFileOut(relative_path, &image_path);
  }

  // Load the icon from file.
  return base::win::ScopedHICON(
      static_cast<HICON>(LoadImage(NULL, image_path.value().c_str(), IMAGE_ICON,
                                   size, size, LR_LOADFROMFILE)));
}
#endif

void Noop(char*, void*) {}

}  // namespace

NativeImage::NativeImage(v8::Isolate* isolate, const gfx::Image& image)
    : image_(image) {
  Init(isolate);
  if (image_.HasRepresentation(gfx::Image::kImageRepSkia)) {
    isolate->AdjustAmountOfExternalAllocatedMemory(
        image_.ToImageSkia()->bitmap()->computeByteSize());
  }
}

#if defined(OS_WIN)
NativeImage::NativeImage(v8::Isolate* isolate, const base::FilePath& hicon_path)
    : hicon_path_(hicon_path) {
  // Use the 256x256 icon as fallback icon.
  gfx::ImageSkia image_skia;
  electron::util::ReadImageSkiaFromICO(&image_skia, GetHICON(256));
  image_ = gfx::Image(image_skia);
  Init(isolate);
  if (image_.HasRepresentation(gfx::Image::kImageRepSkia)) {
    isolate->AdjustAmountOfExternalAllocatedMemory(
        image_.ToImageSkia()->bitmap()->computeByteSize());
  }
}
#endif

NativeImage::~NativeImage() {
  if (image_.HasRepresentation(gfx::Image::kImageRepSkia)) {
    isolate()->AdjustAmountOfExternalAllocatedMemory(-static_cast<int64_t>(
        image_.ToImageSkia()->bitmap()->computeByteSize()));
  }
}

#if defined(OS_WIN)
HICON NativeImage::GetHICON(int size) {
  auto iter = hicons_.find(size);
  if (iter != hicons_.end())
    return iter->second.get();

  // First try loading the icon with specified size.
  if (!hicon_path_.empty()) {
    hicons_[size] = ReadICOFromPath(size, hicon_path_);
    return hicons_[size].get();
  }

  // Then convert the image to ICO.
  if (image_.IsEmpty())
    return NULL;
  hicons_[size] = IconUtil::CreateHICONFromSkBitmap(image_.AsBitmap());
  return hicons_[size].get();
}
#endif

v8::Local<v8::Value> NativeImage::ToPNG(mate::Arguments* args) {
  float scale_factor = GetScaleFactorFromOptions(args);

  if (scale_factor == 1.0f) {
    // Use raw 1x PNG bytes when available
    scoped_refptr<base::RefCountedMemory> png = image_.As1xPNGBytes();
    if (png->size() > 0) {
      const char* data = reinterpret_cast<const char*>(png->front());
      size_t size = png->size();
      return node::Buffer::Copy(args->isolate(), data, size).ToLocalChecked();
    }
  }

  const SkBitmap bitmap =
      image_.AsImageSkia().GetRepresentation(scale_factor).GetBitmap();
  std::vector<unsigned char> encoded;
  gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false, &encoded);
  const char* data = reinterpret_cast<char*>(encoded.data());
  size_t size = encoded.size();
  return node::Buffer::Copy(args->isolate(), data, size).ToLocalChecked();
}

v8::Local<v8::Value> NativeImage::ToBitmap(mate::Arguments* args) {
  float scale_factor = GetScaleFactorFromOptions(args);

  const SkBitmap bitmap =
      image_.AsImageSkia().GetRepresentation(scale_factor).GetBitmap();
  SkPixelRef* ref = bitmap.pixelRef();
  if (!ref)
    return node::Buffer::New(args->isolate(), 0).ToLocalChecked();
  return node::Buffer::Copy(args->isolate(),
                            reinterpret_cast<const char*>(ref->pixels()),
                            bitmap.computeByteSize())
      .ToLocalChecked();
}

v8::Local<v8::Value> NativeImage::ToJPEG(v8::Isolate* isolate, int quality) {
  std::vector<unsigned char> output;
  gfx::JPEG1xEncodedDataFromImage(image_, quality, &output);
  if (output.empty())
    return node::Buffer::New(isolate, 0).ToLocalChecked();
  return node::Buffer::Copy(isolate,
                            reinterpret_cast<const char*>(&output.front()),
                            output.size())
      .ToLocalChecked();
}

std::string NativeImage::ToDataURL(mate::Arguments* args) {
  float scale_factor = GetScaleFactorFromOptions(args);

  if (scale_factor == 1.0f) {
    // Use raw 1x PNG bytes when available
    scoped_refptr<base::RefCountedMemory> png = image_.As1xPNGBytes();
    if (png->size() > 0)
      return webui::GetPngDataUrl(png->front(), png->size());
  }

  return webui::GetBitmapDataUrl(
      image_.AsImageSkia().GetRepresentation(scale_factor).GetBitmap());
}

v8::Local<v8::Value> NativeImage::GetBitmap(mate::Arguments* args) {
  float scale_factor = GetScaleFactorFromOptions(args);

  const SkBitmap bitmap =
      image_.AsImageSkia().GetRepresentation(scale_factor).GetBitmap();
  SkPixelRef* ref = bitmap.pixelRef();
  if (!ref)
    return node::Buffer::New(args->isolate(), 0).ToLocalChecked();
  return node::Buffer::New(args->isolate(),
                           reinterpret_cast<char*>(ref->pixels()),
                           bitmap.computeByteSize(), &Noop, nullptr)
      .ToLocalChecked();
}

v8::Local<v8::Value> NativeImage::GetNativeHandle(v8::Isolate* isolate,
                                                  mate::Arguments* args) {
#if defined(OS_MACOSX)
  if (IsEmpty())
    return node::Buffer::New(isolate, 0).ToLocalChecked();

  NSImage* ptr = image_.AsNSImage();
  return node::Buffer::Copy(isolate, reinterpret_cast<char*>(ptr),
                            sizeof(void*))
      .ToLocalChecked();
#else
  args->ThrowError("Not implemented");
  return v8::Undefined(isolate);
#endif
}

bool NativeImage::IsEmpty() {
  return image_.IsEmpty();
}

gfx::Size NativeImage::GetSize(const base::Optional<float> scale_factor) {
  float sf = scale_factor.value_or(1.0f);
  gfx::ImageSkiaRep image_rep = image_.AsImageSkia().GetRepresentation(sf);
  return gfx::Size(image_rep.GetWidth(), image_rep.GetHeight());
}

std::vector<float> NativeImage::GetScaleFactors() {
  gfx::ImageSkia image_skia = image_.AsImageSkia();
  return image_skia.GetSupportedScales();
}

float NativeImage::GetAspectRatio(const base::Optional<float> scale_factor) {
  float sf = scale_factor.value_or(1.0f);
  gfx::Size size = GetSize(sf);
  if (size.IsEmpty())
    return 1.f;
  else
    return static_cast<float>(size.width()) / static_cast<float>(size.height());
}

mate::Handle<NativeImage> NativeImage::Resize(
    mate::Arguments* args,
    const base::DictionaryValue& options) {
  const float scale_factor = GetScaleFactorFromOptions(args);

  gfx::Size size = GetSize(scale_factor);
  int width = size.width();
  int height = size.height();
  bool width_set = options.GetInteger("width", &width);
  bool height_set = options.GetInteger("height", &height);
  size.SetSize(width, height);

  if (width_set && !height_set) {
    // Scale height to preserve original aspect ratio
    size.set_height(width);
    size =
        gfx::ScaleToRoundedSize(size, 1.f, 1.f / GetAspectRatio(scale_factor));
  } else if (height_set && !width_set) {
    // Scale width to preserve original aspect ratio
    size.set_width(height);
    size = gfx::ScaleToRoundedSize(size, GetAspectRatio(scale_factor), 1.f);
  }

  skia::ImageOperations::ResizeMethod method =
      skia::ImageOperations::ResizeMethod::RESIZE_BEST;
  std::string quality;
  options.GetString("quality", &quality);
  if (quality == "good")
    method = skia::ImageOperations::ResizeMethod::RESIZE_GOOD;
  else if (quality == "better")
    method = skia::ImageOperations::ResizeMethod::RESIZE_BETTER;

  gfx::ImageSkia resized = gfx::ImageSkiaOperations::CreateResizedImage(
      image_.AsImageSkia(), method, size);
  return mate::CreateHandle(
      args->isolate(), new NativeImage(args->isolate(), gfx::Image(resized)));
}

mate::Handle<NativeImage> NativeImage::Crop(v8::Isolate* isolate,
                                            const gfx::Rect& rect) {
  gfx::ImageSkia cropped =
      gfx::ImageSkiaOperations::ExtractSubset(image_.AsImageSkia(), rect);
  return mate::CreateHandle(isolate,
                            new NativeImage(isolate, gfx::Image(cropped)));
}

void NativeImage::AddRepresentation(const mate::Dictionary& options) {
  int width = 0;
  int height = 0;
  float scale_factor = 1.0f;
  options.Get("width", &width);
  options.Get("height", &height);
  options.Get("scaleFactor", &scale_factor);

  bool skia_rep_added = false;
  gfx::ImageSkia image_skia = image_.AsImageSkia();

  v8::Local<v8::Value> buffer;
  GURL url;
  if (options.Get("buffer", &buffer) && node::Buffer::HasInstance(buffer)) {
    auto* data = reinterpret_cast<unsigned char*>(node::Buffer::Data(buffer));
    auto size = node::Buffer::Length(buffer);
    skia_rep_added = electron::util::AddImageSkiaRepFromBuffer(
        &image_skia, data, size, width, height, scale_factor);
  } else if (options.Get("dataURL", &url)) {
    std::string mime_type, charset, data;
    if (net::DataURL::Parse(url, &mime_type, &charset, &data)) {
      auto* data_ptr = reinterpret_cast<const unsigned char*>(data.c_str());
      if (mime_type == "image/png") {
        skia_rep_added = electron::util::AddImageSkiaRepFromPNG(
            &image_skia, data_ptr, data.size(), scale_factor);
      } else if (mime_type == "image/jpeg") {
        skia_rep_added = electron::util::AddImageSkiaRepFromJPEG(
            &image_skia, data_ptr, data.size(), scale_factor);
      }
    }
  }

  // Re-initialize image when first representation is added to an empty image
  if (skia_rep_added && IsEmpty()) {
    gfx::Image image(image_skia);
    image_ = std::move(image);
  }
}

#if !defined(OS_MACOSX)
void NativeImage::SetTemplateImage(bool setAsTemplate) {}

bool NativeImage::IsTemplateImage() {
  return false;
}
#endif

// static
mate::Handle<NativeImage> NativeImage::CreateEmpty(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new NativeImage(isolate, gfx::Image()));
}

// static
mate::Handle<NativeImage> NativeImage::Create(v8::Isolate* isolate,
                                              const gfx::Image& image) {
  return mate::CreateHandle(isolate, new NativeImage(isolate, image));
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromPNG(v8::Isolate* isolate,
                                                     const char* buffer,
                                                     size_t length) {
  gfx::Image image = gfx::Image::CreateFrom1xPNGBytes(
      reinterpret_cast<const unsigned char*>(buffer), length);
  return Create(isolate, image);
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromJPEG(v8::Isolate* isolate,
                                                      const char* buffer,
                                                      size_t length) {
  gfx::Image image = gfx::ImageFrom1xJPEGEncodedData(
      reinterpret_cast<const unsigned char*>(buffer), length);
  return Create(isolate, image);
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromPath(
    v8::Isolate* isolate,
    const base::FilePath& path) {
  base::FilePath image_path = NormalizePath(path);
#if defined(OS_WIN)
  if (image_path.MatchesExtension(FILE_PATH_LITERAL(".ico"))) {
    return mate::CreateHandle(isolate, new NativeImage(isolate, image_path));
  }
#endif
  gfx::ImageSkia image_skia;
  electron::util::PopulateImageSkiaRepsFromPath(&image_skia, image_path);
  gfx::Image image(image_skia);
  mate::Handle<NativeImage> handle = Create(isolate, image);
#if defined(OS_MACOSX)
  if (IsTemplateFilename(image_path))
    handle->SetTemplateImage(true);
#endif
  return handle;
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromBitmap(
    mate::Arguments* args,
    v8::Local<v8::Value> buffer,
    const mate::Dictionary& options) {
  if (!node::Buffer::HasInstance(buffer)) {
    args->ThrowError("buffer must be a node Buffer");
    return mate::Handle<NativeImage>();
  }

  unsigned int width = 0;
  unsigned int height = 0;
  double scale_factor = 1.;

  if (!options.Get("width", &width)) {
    args->ThrowError("width is required");
    return mate::Handle<NativeImage>();
  }

  if (!options.Get("height", &height)) {
    args->ThrowError("height is required");
    return mate::Handle<NativeImage>();
  }

  auto info = SkImageInfo::MakeN32(width, height, kPremul_SkAlphaType);
  auto size_bytes = info.computeMinByteSize();

  if (size_bytes != node::Buffer::Length(buffer)) {
    args->ThrowError("invalid buffer size");
    return mate::Handle<NativeImage>();
  }

  options.Get("scaleFactor", &scale_factor);

  if (width == 0 || height == 0) {
    return CreateEmpty(args->isolate());
  }

  SkBitmap bitmap;
  bitmap.allocN32Pixels(width, height, false);
  bitmap.writePixels({info, node::Buffer::Data(buffer), bitmap.rowBytes()});

  gfx::ImageSkia image_skia;
  image_skia.AddRepresentation(gfx::ImageSkiaRep(bitmap, scale_factor));

  return Create(args->isolate(), gfx::Image(image_skia));
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromBuffer(
    mate::Arguments* args,
    v8::Local<v8::Value> buffer) {
  if (!node::Buffer::HasInstance(buffer)) {
    args->ThrowError("buffer must be a node Buffer");
    return mate::Handle<NativeImage>();
  }

  int width = 0;
  int height = 0;
  double scale_factor = 1.;

  mate::Dictionary options;
  if (args->GetNext(&options)) {
    options.Get("width", &width);
    options.Get("height", &height);
    options.Get("scaleFactor", &scale_factor);
  }

  gfx::ImageSkia image_skia;
  electron::util::AddImageSkiaRepFromBuffer(
      &image_skia, reinterpret_cast<unsigned char*>(node::Buffer::Data(buffer)),
      node::Buffer::Length(buffer), width, height, scale_factor);
  return Create(args->isolate(), gfx::Image(image_skia));
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromDataURL(v8::Isolate* isolate,
                                                         const GURL& url) {
  std::string mime_type, charset, data;
  if (net::DataURL::Parse(url, &mime_type, &charset, &data)) {
    if (mime_type == "image/png")
      return CreateFromPNG(isolate, data.c_str(), data.size());
    else if (mime_type == "image/jpeg")
      return CreateFromJPEG(isolate, data.c_str(), data.size());
  }

  return CreateEmpty(isolate);
}

#if !defined(OS_MACOSX)
mate::Handle<NativeImage> NativeImage::CreateFromNamedImage(
    mate::Arguments* args,
    const std::string& name) {
  return CreateEmpty(args->isolate());
}
#endif

// static
void NativeImage::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "NativeImage"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("toPNG", &NativeImage::ToPNG)
      .SetMethod("toJPEG", &NativeImage::ToJPEG)
      .SetMethod("toBitmap", &NativeImage::ToBitmap)
      .SetMethod("getBitmap", &NativeImage::GetBitmap)
      .SetMethod("getScaleFactors", &NativeImage::GetScaleFactors)
      .SetMethod("getNativeHandle", &NativeImage::GetNativeHandle)
      .SetMethod("toDataURL", &NativeImage::ToDataURL)
      .SetMethod("isEmpty", &NativeImage::IsEmpty)
      .SetMethod("getSize", &NativeImage::GetSize)
      .SetMethod("setTemplateImage", &NativeImage::SetTemplateImage)
      .SetMethod("isTemplateImage", &NativeImage::IsTemplateImage)
      .SetProperty("isMacTemplateImage", &NativeImage::IsTemplateImage,
                   &NativeImage::SetTemplateImage)
      .SetMethod("resize", &NativeImage::Resize)
      .SetMethod("crop", &NativeImage::Crop)
      .SetMethod("getAspectRatio", &NativeImage::GetAspectRatio)
      .SetMethod("addRepresentation", &NativeImage::AddRepresentation);
}

}  // namespace api

}  // namespace electron

namespace mate {

v8::Local<v8::Value> Converter<mate::Handle<electron::api::NativeImage>>::ToV8(
    v8::Isolate* isolate,
    const mate::Handle<electron::api::NativeImage>& val) {
  return val.ToV8();
}

bool Converter<mate::Handle<electron::api::NativeImage>>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    mate::Handle<electron::api::NativeImage>* out) {
  // Try converting from file path.
  base::FilePath path;
  if (ConvertFromV8(isolate, val, &path)) {
    *out = electron::api::NativeImage::CreateFromPath(isolate, path);
    // Should throw when failed to initialize from path.
    return !(*out)->image().IsEmpty();
  }

  WrappableBase* wrapper =
      static_cast<WrappableBase*>(internal::FromV8Impl(isolate, val));
  if (!wrapper)
    return false;

  *out =
      CreateHandle(isolate, static_cast<electron::api::NativeImage*>(wrapper));
  return true;
}

}  // namespace mate

namespace {

using electron::api::NativeImage;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("NativeImage", NativeImage::GetConstructor(isolate)
                              ->GetFunction(context)
                              .ToLocalChecked());
  mate::Dictionary native_image = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("nativeImage", native_image);

  native_image.SetMethod("createEmpty", &NativeImage::CreateEmpty);
  native_image.SetMethod("createFromPath", &NativeImage::CreateFromPath);
  native_image.SetMethod("createFromBitmap", &NativeImage::CreateFromBitmap);
  native_image.SetMethod("createFromBuffer", &NativeImage::CreateFromBuffer);
  native_image.SetMethod("createFromDataURL", &NativeImage::CreateFromDataURL);
  native_image.SetMethod("createFromNamedImage",
                         &NativeImage::CreateFromNamedImage);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_common_native_image, Initialize)
