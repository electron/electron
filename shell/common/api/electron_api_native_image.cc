// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_native_image.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/strings/pattern.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "net/base/data_url.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "shell/common/skia_util.h"
#include "skia/ext/platform_canvas.h"
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
float GetScaleFactorFromOptions(gin::Arguments* args) {
  float scale_factor = 1.0f;
  gin_helper::Dictionary options;
  if (args->GetNext(&options))
    options.Get("scaleFactor", &scale_factor);
  return scale_factor;
}

base::FilePath NormalizePath(const base::FilePath& path) {
  if (!path.ReferencesParent()) {
    return path;
  }

  base::ThreadRestrictions::ScopedAllowIO allow_blocking;
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

int kPillFontMeasuringSize = 100;

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

v8::Local<v8::Value> NativeImage::ToPNG(gin::Arguments* args) {
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

v8::Local<v8::Value> NativeImage::ToBitmap(gin::Arguments* args) {
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

std::string NativeImage::ToDataURL(gin::Arguments* args) {
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

v8::Local<v8::Value> NativeImage::GetBitmap(gin::Arguments* args) {
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

v8::Local<v8::Value> NativeImage::GetNativeHandle(
    gin_helper::ErrorThrower thrower) {
#if defined(OS_MACOSX)
  if (IsEmpty())
    return node::Buffer::New(thrower.isolate(), 0).ToLocalChecked();

  NSImage* ptr = image_.AsNSImage();
  return node::Buffer::Copy(thrower.isolate(), reinterpret_cast<char*>(ptr),
                            sizeof(void*))
      .ToLocalChecked();
#else
  thrower.ThrowError("Not implemented");
  return v8::Undefined(thrower.isolate());
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

gin::Handle<NativeImage> NativeImage::Resize(gin::Arguments* args,
                                             base::DictionaryValue options) {
  float scale_factor = GetScaleFactorFromOptions(args);

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
  return gin::CreateHandle(
      args->isolate(), new NativeImage(args->isolate(), gfx::Image(resized)));
}

gin::Handle<NativeImage> NativeImage::Crop(v8::Isolate* isolate,
                                           const gfx::Rect& rect) {
  gfx::ImageSkia cropped =
      gfx::ImageSkiaOperations::ExtractSubset(image_.AsImageSkia(), rect);
  return gin::CreateHandle(isolate,
                           new NativeImage(isolate, gfx::Image(cropped)));
}

void NativeImage::AddRepresentation(const gin_helper::Dictionary& options) {
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
gin::Handle<NativeImage> NativeImage::CreateEmpty(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new NativeImage(isolate, gfx::Image()));
}

// static
gin::Handle<NativeImage> NativeImage::Create(v8::Isolate* isolate,
                                             const gfx::Image& image) {
  return gin::CreateHandle(isolate, new NativeImage(isolate, image));
}

// static
gin::Handle<NativeImage> NativeImage::CreateFromPNG(v8::Isolate* isolate,
                                                    const char* buffer,
                                                    size_t length) {
  gfx::Image image = gfx::Image::CreateFrom1xPNGBytes(
      reinterpret_cast<const unsigned char*>(buffer), length);
  return Create(isolate, image);
}

// static
gin::Handle<NativeImage> NativeImage::CreateFromJPEG(v8::Isolate* isolate,
                                                     const char* buffer,
                                                     size_t length) {
  gfx::Image image = gfx::ImageFrom1xJPEGEncodedData(
      reinterpret_cast<const unsigned char*>(buffer), length);
  return Create(isolate, image);
}

// static
gin::Handle<NativeImage> NativeImage::CreateFromPath(
    v8::Isolate* isolate,
    const base::FilePath& path) {
  base::FilePath image_path = NormalizePath(path);
#if defined(OS_WIN)
  if (image_path.MatchesExtension(FILE_PATH_LITERAL(".ico"))) {
    return gin::CreateHandle(isolate, new NativeImage(isolate, image_path));
  }
#endif
  gfx::ImageSkia image_skia;
  electron::util::PopulateImageSkiaRepsFromPath(&image_skia, image_path);
  gfx::Image image(image_skia);
  gin::Handle<NativeImage> handle = Create(isolate, image);
#if defined(OS_MACOSX)
  if (IsTemplateFilename(image_path))
    handle->SetTemplateImage(true);
#endif
  return handle;
}

// static
gin::Handle<NativeImage> NativeImage::CreateFromBitmap(
    gin_helper::ErrorThrower thrower,
    v8::Local<v8::Value> buffer,
    const gin_helper::Dictionary& options) {
  if (!node::Buffer::HasInstance(buffer)) {
    thrower.ThrowError("buffer must be a node Buffer");
    return gin::Handle<NativeImage>();
  }

  unsigned int width = 0;
  unsigned int height = 0;
  double scale_factor = 1.;

  if (!options.Get("width", &width)) {
    thrower.ThrowError("width is required");
    return gin::Handle<NativeImage>();
  }

  if (!options.Get("height", &height)) {
    thrower.ThrowError("height is required");
    return gin::Handle<NativeImage>();
  }

  auto info = SkImageInfo::MakeN32(width, height, kPremul_SkAlphaType);
  auto size_bytes = info.computeMinByteSize();

  if (size_bytes != node::Buffer::Length(buffer)) {
    thrower.ThrowError("invalid buffer size");
    return gin::Handle<NativeImage>();
  }

  options.Get("scaleFactor", &scale_factor);

  if (width == 0 || height == 0) {
    return CreateEmpty(thrower.isolate());
  }

  SkBitmap bitmap;
  bitmap.allocN32Pixels(width, height, false);
  bitmap.writePixels({info, node::Buffer::Data(buffer), bitmap.rowBytes()});

  gfx::ImageSkia image_skia;
  image_skia.AddRepresentation(gfx::ImageSkiaRep(bitmap, scale_factor));

  return Create(thrower.isolate(), gfx::Image(image_skia));
}

// static
gin::Handle<NativeImage> NativeImage::CreateFromBuffer(
    gin_helper::ErrorThrower thrower,
    v8::Local<v8::Value> buffer,
    gin::Arguments* args) {
  if (!node::Buffer::HasInstance(buffer)) {
    thrower.ThrowError("buffer must be a node Buffer");
    return gin::Handle<NativeImage>();
  }

  int width = 0;
  int height = 0;
  double scale_factor = 1.;

  gin_helper::Dictionary options;
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
gin::Handle<NativeImage> NativeImage::CreateFromDataURL(v8::Isolate* isolate,
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
gin::Handle<NativeImage> NativeImage::CreateFromNamedImage(gin::Arguments* args,
                                                           std::string name) {
  return CreateEmpty(args->isolate());
}
#endif

gin::Handle<NativeImage> NativeImage::AddBadge(gin_helper::ErrorThrower thrower,
                                               gin_helper::Dictionary options) {
  if (IsEmpty())
    return CreateEmpty(thrower.isolate());

  v8::Isolate* isolate = thrower.isolate();
  std::string text;
  std::string font_family = "monospace";
  SkColor text_color = SK_ColorWHITE;
  SkColor pill_color = SK_ColorRED;
  if (!options.Get("text", &text)) {
    thrower.ThrowError("Required option 'text' was not provided to addBadge");
    return gin::Handle<NativeImage>();
  }
  options.Get("font", &font_family);
  {
    std::string text_hex_code;
    if (options.Get("textColor", &text_hex_code)) {
      text_color = ParseHexColor(text_hex_code);
    }
  }
  {
    std::string pill_hex_code;
    if (options.Get("badgeColor", &pill_hex_code)) {
      pill_color = ParseHexColor(pill_hex_code);
    }
  }
  BadgePosition badge_position = BadgePosition::TOP_RIGHT;
  options.Get("badgePosition", &badge_position);

  SkFont font;
  SkTextEncoding encoding = SkTextEncoding::kUTF8;
  font.setTypeface(
      SkTypeface::MakeFromName(font_family.c_str(), SkFontStyle()));
  font.setHinting(SkFontHinting::kNormal);
  font.setSize(kPillFontMeasuringSize);

  SkPaint text_paint;
  text_paint.setColor(text_color);
  text_paint.setStyle(SkPaint::kFill_Style);

  SkRect sample_text_bounds;
  const std::string sample_text = "1";
  font.measureText(sample_text.c_str(), sample_text.length(), encoding,
                   &sample_text_bounds, &text_paint);

  SkScalar pt_ratio = kPillFontMeasuringSize / sample_text_bounds.height();

  SkPaint pill_paint;
  pill_paint.setColor(pill_color);
  pill_paint.setStyle(SkPaint::kFill_Style);

  SkBitmap current_bitmap =
      image_.AsImageSkia().GetRepresentation(1.0f).GetBitmap();
  SkBitmap bitmap;
  bitmap.setInfo(current_bitmap.info());
  bitmap.allocPixels(current_bitmap.info());
  bitmap.writePixels(current_bitmap.pixmap(), 0, 0);
  SkCanvas canvas(bitmap);

  SkScalar target_pill_height = current_bitmap.height() * 380.f / 1024.f;
  SkScalar target_text_height = target_pill_height * 9.f / 24.f;
  SkScalar pill_radius = target_pill_height / 2;
  font.setSize(pt_ratio * target_text_height);

  SkRect text_bounds;
  font.measureText(text.c_str(), text.length(), encoding, &text_bounds,
                   &text_paint);

  SkScalar target_pill_width = text_bounds.width() + (pill_radius * 4 / 7);
  if (target_pill_width < target_pill_height) {
    target_pill_width = target_pill_height;
  }
  SkScalar text_offset_x = (target_pill_width - text_bounds.width()) / 2;
  SkScalar text_offset_y = (target_pill_height - text_bounds.height()) / 2;

  SkScalar badge_left;
  SkScalar badge_top;
  switch (badge_position) {
    case BadgePosition::TOP_LEFT:
      badge_left = 0;
      badge_top = 0;
      break;
    case BadgePosition::TOP_RIGHT:
      badge_left = current_bitmap.width() - target_pill_width;
      badge_top = 0;
      break;
    case BadgePosition::BOTTOM_LEFT:
      badge_left = 0;
      badge_top = current_bitmap.height() - target_pill_height;
      break;
    case BadgePosition::BOTTOM_RIGHT:
      badge_left = current_bitmap.width() - target_pill_width;
      badge_top = current_bitmap.height() - target_pill_height;
      break;
  }

  canvas.drawCircle(badge_left + pill_radius, badge_top + pill_radius,
                    pill_radius, pill_paint);
  if (target_pill_width > target_pill_height) {
    canvas.drawCircle(badge_left + target_pill_width - pill_radius,
                      badge_top + pill_radius, pill_radius, pill_paint);
    SkRect rect = SkRect::MakeLTRB(badge_left + pill_radius, badge_top,
                                   badge_left + target_pill_width - pill_radius,
                                   badge_top + target_pill_height);
    canvas.drawRect(rect, pill_paint);
  }

  auto blob = SkTextBlob::MakeFromString(text.c_str(), font, encoding);
  canvas.drawTextBlob(blob, badge_left + text_offset_x - text_bounds.x(),
                      badge_top - text_bounds.y() + text_offset_y, text_paint);

  gfx::ImageSkia badged;
  badged.AddRepresentation(gfx::ImageSkiaRep(bitmap, 1.0f));

  return Create(isolate, gfx::Image(badged));
}

// static
void NativeImage::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "NativeImage"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
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
      .SetMethod("addBadge", &NativeImage::AddBadge)
      .SetMethod("getAspectRatio", &NativeImage::GetAspectRatio)
      .SetMethod("addRepresentation", &NativeImage::AddRepresentation);
}

}  // namespace api

}  // namespace electron

namespace gin {

v8::Local<v8::Value> Converter<electron::api::NativeImage*>::ToV8(
    v8::Isolate* isolate,
    electron::api::NativeImage* val) {
  if (val)
    return val->GetWrapper();
  else
    return v8::Null(isolate);
}

bool Converter<electron::api::NativeImage*>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    electron::api::NativeImage** out) {
  // Try converting from file path.
  base::FilePath path;
  if (ConvertFromV8(isolate, val, &path)) {
    *out = electron::api::NativeImage::CreateFromPath(isolate, path).get();
    if ((*out)->image().IsEmpty()) {
#if defined(OS_WIN)
      const auto img_path = base::UTF16ToUTF8(path.value());
#else
      const auto img_path = path.value();
#endif
      isolate->ThrowException(v8::Exception::Error(
          StringToV8(isolate, "Image could not be created from " + img_path)));
      return false;
    }

    return true;
  }

  *out = static_cast<electron::api::NativeImage*>(
      static_cast<gin_helper::WrappableBase*>(
          gin_helper::internal::FromV8Impl(isolate, val)));
  return *out != nullptr;
}

bool Converter<electron::api::BadgePosition>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    electron::api::BadgePosition* out) {
  std::string position;
  if (!gin::ConvertFromV8(isolate, val, &position))
    return false;
  if (position == "top-left") {
    *out = electron::api::BadgePosition::TOP_LEFT;
    return true;
  } else if (position == "top-right") {
    *out = electron::api::BadgePosition::TOP_RIGHT;
    return true;
  } else if (position == "bottom-left") {
    *out = electron::api::BadgePosition::BOTTOM_LEFT;
    return true;
  } else if (position == "bottom-right") {
    *out = electron::api::BadgePosition::BOTTOM_RIGHT;
    return true;
  }

  return false;
}

}  // namespace gin

namespace {

using electron::api::NativeImage;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("NativeImage", NativeImage::GetConstructor(isolate)
                              ->GetFunction(context)
                              .ToLocalChecked());
  gin_helper::Dictionary native_image = gin::Dictionary::CreateEmpty(isolate);
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

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_native_image, Initialize)
