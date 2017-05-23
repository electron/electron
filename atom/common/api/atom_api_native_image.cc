// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_native_image.h"

#include <string>
#include <vector>

#include "atom/common/asar/asar_util.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/files/file_util.h"
#include "base/strings/pattern.h"
#include "base/strings/string_util.h"
#include "native_mate/object_template_builder.h"
#include "net/base/data_url.h"
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
#include "atom/common/asar/archive.h"
#include "base/win/scoped_gdi_object.h"
#include "ui/gfx/icon_util.h"
#endif

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

struct ScaleFactorPair {
  const char* name;
  float scale;
};

ScaleFactorPair kScaleFactorPairs[] = {
  // The "@2x" is put as first one to make scale matching faster.
  { "@2x"    , 2.0f },
  { "@3x"    , 3.0f },
  { "@1x"    , 1.0f },
  { "@4x"    , 4.0f },
  { "@5x"    , 5.0f },
  { "@1.25x" , 1.25f },
  { "@1.33x" , 1.33f },
  { "@1.4x"  , 1.4f },
  { "@1.5x"  , 1.5f },
  { "@1.8x"  , 1.8f },
  { "@2.5x"  , 2.5f },
};

float GetScaleFactorFromPath(const base::FilePath& path) {
  std::string filename(path.BaseName().RemoveExtension().AsUTF8Unsafe());

  // We don't try to convert string to float here because it is very very
  // expensive.
  for (const auto& kScaleFactorPair : kScaleFactorPairs) {
    if (base::EndsWith(filename, kScaleFactorPair.name,
                       base::CompareCase::INSENSITIVE_ASCII))
      return kScaleFactorPair.scale;
  }

  return 1.0f;
}

// Get the scale factor from options object at the first argument
float GetScaleFactorFromOptions(mate::Arguments* args) {
  float scale_factor = 1.0f;
  mate::Dictionary options;
  if (args->GetNext(&options))
    options.Get("scaleFactor", &scale_factor);
  return scale_factor;
}

bool AddImageSkiaRep(gfx::ImageSkia* image,
                     const unsigned char* data,
                     size_t size,
                     int width,
                     int height,
                     double scale_factor) {
  std::unique_ptr<SkBitmap> decoded(new SkBitmap());

  // Try PNG first.
  if (!gfx::PNGCodec::Decode(data, size, decoded.get()))
    // Try JPEG.
    decoded = gfx::JPEGCodec::Decode(data, size);

  if (!decoded) {
    // Try Bitmap
    if (width > 0 && height > 0) {
      decoded.reset(new SkBitmap);
      decoded->allocN32Pixels(width, height, false);
      decoded->setPixels(
        const_cast<void*>(reinterpret_cast<const void*>(data)));
    } else {
      return false;
    }
  }

  image->AddRepresentation(gfx::ImageSkiaRep(*decoded, scale_factor));
  return true;
}

bool AddImageSkiaRep(gfx::ImageSkia* image,
                     const base::FilePath& path,
                     double scale_factor) {
  std::string file_contents;
  if (!asar::ReadFileToString(path, &file_contents))
    return false;

  const unsigned char* data =
      reinterpret_cast<const unsigned char*>(file_contents.data());
  size_t size = file_contents.size();
  return AddImageSkiaRep(image, data, size, 0, 0, scale_factor);
}

bool PopulateImageSkiaRepsFromPath(gfx::ImageSkia* image,
                                   const base::FilePath& path) {
  bool succeed = false;
  std::string filename(path.BaseName().RemoveExtension().AsUTF8Unsafe());
  if (base::MatchPattern(filename, "*@*x"))
    // Don't search for other representations if the DPI has been specified.
    return AddImageSkiaRep(image, path, GetScaleFactorFromPath(path));
  else
    succeed |= AddImageSkiaRep(image, path, 1.0f);

  for (const ScaleFactorPair& pair : kScaleFactorPairs)
    succeed |= AddImageSkiaRep(image,
                               path.InsertBeforeExtensionASCII(pair.name),
                               pair.scale);
  return succeed;
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
  return base::win::ScopedHICON(static_cast<HICON>(
      LoadImage(NULL, image_path.value().c_str(), IMAGE_ICON, size, size,
                LR_LOADFROMFILE)));
}

bool ReadImageSkiaFromICO(gfx::ImageSkia* image, HICON icon) {
  // Convert the icon from the Windows specific HICON to gfx::ImageSkia.
  std::unique_ptr<SkBitmap> bitmap(IconUtil::CreateSkBitmapFromHICON(icon));
  if (!bitmap)
    return false;

  image->AddRepresentation(gfx::ImageSkiaRep(*bitmap, 1.0f));
  return true;
}
#endif

void Noop(char*, void*) {
}

}  // namespace

NativeImage::NativeImage(v8::Isolate* isolate, const gfx::Image& image)
    : image_(image) {
  Init(isolate);
  if (image_.HasRepresentation(gfx::Image::kImageRepSkia)) {
    isolate->AdjustAmountOfExternalAllocatedMemory(
      image_.ToImageSkia()->bitmap()->computeSize64());
  }
  MarkHighMemoryUsage();
}

#if defined(OS_WIN)
NativeImage::NativeImage(v8::Isolate* isolate, const base::FilePath& hicon_path)
    : hicon_path_(hicon_path) {
  // Use the 256x256 icon as fallback icon.
  gfx::ImageSkia image_skia;
  ReadImageSkiaFromICO(&image_skia, GetHICON(256));
  image_ = gfx::Image(image_skia);
  Init(isolate);
  if (image_.HasRepresentation(gfx::Image::kImageRepSkia)) {
    isolate->AdjustAmountOfExternalAllocatedMemory(
      image_.ToImageSkia()->bitmap()->computeSize64());
  }
  MarkHighMemoryUsage();
}
#endif

NativeImage::~NativeImage() {
  if (image_.HasRepresentation(gfx::Image::kImageRepSkia)) {
    isolate()->AdjustAmountOfExternalAllocatedMemory(
      - image_.ToImageSkia()->bitmap()->computeSize64());
  }
}

#if defined(OS_WIN)
HICON NativeImage::GetHICON(int size) {
  auto iter = hicons_.find(size);
  if (iter != hicons_.end())
    return iter->second.get();

  // First try loading the icon with specified size.
  if (!hicon_path_.empty()) {
    hicons_[size] = std::move(ReadICOFromPath(size, hicon_path_));
    return hicons_[size].get();
  }

  // Then convert the image to ICO.
  if (image_.IsEmpty())
    return NULL;
  hicons_[size] = std::move(
      IconUtil::CreateHICONFromSkBitmap(image_.AsBitmap()));
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
      image_.AsImageSkia().GetRepresentation(scale_factor).sk_bitmap();
  std::unique_ptr<std::vector<unsigned char>> encoded(
      new std::vector<unsigned char>());
  gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false, encoded.get());
  const char* data = reinterpret_cast<char*>(encoded->data());
  size_t size = encoded->size();
  return node::Buffer::Copy(args->isolate(), data, size).ToLocalChecked();
}

v8::Local<v8::Value> NativeImage::ToBitmap(mate::Arguments* args) {
  float scale_factor = GetScaleFactorFromOptions(args);

  const SkBitmap bitmap =
      image_.AsImageSkia().GetRepresentation(scale_factor).sk_bitmap();
  SkPixelRef* ref = bitmap.pixelRef();
  if (!ref)
    return node::Buffer::New(args->isolate(), 0).ToLocalChecked();
  return node::Buffer::Copy(args->isolate(),
                            reinterpret_cast<const char*>(ref->pixels()),
                            bitmap.getSafeSize()).ToLocalChecked();
}

v8::Local<v8::Value> NativeImage::ToJPEG(v8::Isolate* isolate, int quality) {
  std::vector<unsigned char> output;
  gfx::JPEG1xEncodedDataFromImage(image_, quality, &output);
  return node::Buffer::Copy(
      isolate,
      reinterpret_cast<const char*>(&output.front()),
      output.size()).ToLocalChecked();
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
      image_.AsImageSkia().GetRepresentation(scale_factor).sk_bitmap());
}

v8::Local<v8::Value> NativeImage::GetBitmap(mate::Arguments* args) {
  float scale_factor = GetScaleFactorFromOptions(args);

  const SkBitmap bitmap =
      image_.AsImageSkia().GetRepresentation(scale_factor).sk_bitmap();
  SkPixelRef* ref = bitmap.pixelRef();
  if (!ref){
    return node::Buffer::New(args->isolate(), 0).ToLocalChecked();
  }

  	return node::Buffer::New(args->isolate(),
                         reinterpret_cast<char*>(ref->pixels()),
                         bitmap.getSafeSize(),
                         &Noop,
                         nullptr).ToLocalChecked();
    
}

v8::Local<v8::Value> NativeImage::GetBitmapMonochrome(mate::Arguments* args) {
  float scale_factor = GetScaleFactorFromOptions(args);

  const SkBitmap bitmap =
      image_.AsImageSkia().GetRepresentation(scale_factor).sk_bitmap();
  SkPixelRef* ref = bitmap.pixelRef();
  if (!ref){
    return node::Buffer::New(args->isolate(), 0).ToLocalChecked();
  }
  
  	int width = bitmap.width();
  	int height = bitmap.height();
	int size = width*height;
	int monochromeBitmapBytes = size/8;
	unsigned char* monochromeBitmap = new unsigned char[monochromeBitmapBytes]();

	int bytesPerPixel = bitmap.bytesPerPixel();
	if(bytesPerPixel != 4){
		args->ThrowError("unsupported bytes per pixel");
			return v8::Undefined(args->isolate());
	}

	char *pixels = (char*)ref->pixels();
	for(int i=0; i <size*4; i+=4){
		int pixelNumber = i/4;
		unsigned char b = pixels[i+0];
		unsigned char g = pixels[i+1];
		unsigned char r = pixels[i+2];
		unsigned char a = pixels[i+3];

		bool blackOrWhite = 0;

		if (a > 126) { // checking transparency
            int grayscale = 0.2126 * r + 0.7152 * g + 0.0722 * b;

            if (grayscale < 128) { // checking color
                blackOrWhite = 1;
            }
        }

        int memorySlot = pixelNumber/8;

        if(blackOrWhite){
        	unsigned char slot = monochromeBitmap[memorySlot];
        	unsigned char mask = 1 << (7 - (pixelNumber%8));
        	slot = slot | mask;
        	monochromeBitmap[memorySlot] = slot;
        }
	}

	return node::Buffer::New(args->isolate(),
	                       reinterpret_cast<char*>(monochromeBitmap),
	                       monochromeBitmapBytes,
	                       &Noop,
	                       nullptr).ToLocalChecked();
}

v8::Local<v8::Value> NativeImage::GetNativeHandle(v8::Isolate* isolate,
                                                  mate::Arguments* args) {
#if defined(OS_MACOSX)
  if (IsEmpty()) return node::Buffer::New(isolate, 0).ToLocalChecked();

  NSImage* ptr = image_.AsNSImage();
  return node::Buffer::Copy(
      isolate,
      reinterpret_cast<char*>(ptr),
      sizeof(void*)).ToLocalChecked();
#else
  args->ThrowError("Not implemented");
  return v8::Undefined(isolate);
#endif
}

bool NativeImage::IsEmpty() {
  return image_.IsEmpty();
}

gfx::Size NativeImage::GetSize() {
  return image_.Size();
}

float NativeImage::GetAspectRatio() {
  gfx::Size size = GetSize();
  if (size.IsEmpty())
    return 1.f;
  else
    return static_cast<float>(size.width()) / static_cast<float>(size.height());
}

mate::Handle<NativeImage> NativeImage::Resize(
    v8::Isolate* isolate, const base::DictionaryValue& options) {
  gfx::Size size = GetSize();
  int width = size.width();
  int height = size.height();
  bool width_set = options.GetInteger("width", &width);
  bool height_set = options.GetInteger("height", &height);
  size.SetSize(width, height);

  if (width_set && !height_set) {
    // Scale height to preserve original aspect ratio
    size.set_height(width);
    size = gfx::ScaleToRoundedSize(size, 1.f, 1.f / GetAspectRatio());
  } else if (height_set && !width_set) {
    // Scale width to preserve original aspect ratio
    size.set_width(height);
    size = gfx::ScaleToRoundedSize(size, GetAspectRatio(), 1.f);
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
  return mate::CreateHandle(isolate,
                            new NativeImage(isolate, gfx::Image(resized)));
}

mate::Handle<NativeImage> NativeImage::Crop(v8::Isolate* isolate,
                                            const gfx::Rect& rect) {
  gfx::ImageSkia cropped = gfx::ImageSkiaOperations::ExtractSubset(
      image_.AsImageSkia(), rect);
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
    AddImageSkiaRep(
        &image_skia,
        reinterpret_cast<unsigned char*>(node::Buffer::Data(buffer)),
        node::Buffer::Length(buffer),
        width, height, scale_factor);
    skia_rep_added = true;
  } else if (options.Get("dataURL", &url)) {
    std::string mime_type, charset, data;
    if (net::DataURL::Parse(url, &mime_type, &charset, &data)) {
      if (mime_type == "image/png" || mime_type == "image/jpeg") {
        AddImageSkiaRep(
            &image_skia,
            reinterpret_cast<const unsigned char*>(data.c_str()),
            data.size(),
            width, height, scale_factor);
        skia_rep_added = true;
      }
    }
  }

  // Re-initialize image when first representation is added to an empty image
  if (skia_rep_added && IsEmpty()) {
    gfx::Image image(image_skia);
    image_.SwapRepresentations(&image);
  }
}

#if !defined(OS_MACOSX)
void NativeImage::SetTemplateImage(bool setAsTemplate) {
}

bool NativeImage::IsTemplateImage() {
  return false;
}
#endif

// static
mate::Handle<NativeImage> NativeImage::CreateEmpty(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new NativeImage(isolate, gfx::Image()));
}

// static
mate::Handle<NativeImage> NativeImage::Create(
    v8::Isolate* isolate, const gfx::Image& image) {
  return mate::CreateHandle(isolate, new NativeImage(isolate, image));
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromPNG(
    v8::Isolate* isolate, const char* buffer, size_t length) {
  gfx::Image image = gfx::Image::CreateFrom1xPNGBytes(
      reinterpret_cast<const unsigned char*>(buffer), length);
  return Create(isolate, image);
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromJPEG(
    v8::Isolate* isolate, const char* buffer, size_t length) {
  gfx::Image image = gfx::ImageFrom1xJPEGEncodedData(
      reinterpret_cast<const unsigned char*>(buffer), length);
  return Create(isolate, image);
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromPath(
    v8::Isolate* isolate, const base::FilePath& path) {
  base::FilePath image_path = NormalizePath(path);
#if defined(OS_WIN)
  if (image_path.MatchesExtension(FILE_PATH_LITERAL(".ico"))) {
    return mate::CreateHandle(isolate,
                              new NativeImage(isolate, image_path));
  }
#endif
  gfx::ImageSkia image_skia;
  PopulateImageSkiaRepsFromPath(&image_skia, image_path);
  gfx::Image image(image_skia);
  mate::Handle<NativeImage> handle = Create(isolate, image);
#if defined(OS_MACOSX)
  if (IsTemplateFilename(image_path))
    handle->SetTemplateImage(true);
#endif
  return handle;
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromBuffer(
    mate::Arguments* args, v8::Local<v8::Value> buffer) {
  int width = 0;
  int height = 0;
  double scale_factor = 1.;

  mate::Dictionary options;
  if (args->GetNext(&options)) {
    options.Get("width", &width);
    options.Get("height", &height);
    options.Get("scaleFactor", &scale_factor);
  } else {
    // TODO(kevinsawicki): Remove in 2.0, deprecate before then with warnings
    args->GetNext(&scale_factor);
  }

  gfx::ImageSkia image_skia;
  AddImageSkiaRep(&image_skia,
                  reinterpret_cast<unsigned char*>(node::Buffer::Data(buffer)),
                  node::Buffer::Length(buffer),
                  width,
                  height,
                  scale_factor);
  return Create(args->isolate(), gfx::Image(image_skia));
}

// static
mate::Handle<NativeImage> NativeImage::CreateFromDataURL(
    v8::Isolate* isolate, const GURL& url) {
  std::string mime_type, charset, data;
  if (net::DataURL::Parse(url, &mime_type, &charset, &data)) {
    if (mime_type == "image/png")
      return CreateFromPNG(isolate, data.c_str(), data.size());
    else if (mime_type == "image/jpeg")
      return CreateFromJPEG(isolate, data.c_str(), data.size());
  }

  return CreateEmpty(isolate);
}

// static
void NativeImage::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "NativeImage"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("toPNG", &NativeImage::ToPNG)
      .SetMethod("toJPEG", &NativeImage::ToJPEG)
      .SetMethod("toBitmap", &NativeImage::ToBitmap)
      .SetMethod("getBitmap", &NativeImage::GetBitmap)
      .SetMethod("getBitmapMonochrome", &NativeImage::GetBitmapMonochrome)
      .SetMethod("getNativeHandle", &NativeImage::GetNativeHandle)
      .SetMethod("toDataURL", &NativeImage::ToDataURL)
      .SetMethod("isEmpty", &NativeImage::IsEmpty)
      .SetMethod("getSize", &NativeImage::GetSize)
      .SetMethod("setTemplateImage", &NativeImage::SetTemplateImage)
      .SetMethod("isTemplateImage", &NativeImage::IsTemplateImage)
      .SetMethod("resize", &NativeImage::Resize)
      .SetMethod("crop", &NativeImage::Crop)
      .SetMethod("getAspectRatio", &NativeImage::GetAspectRatio)
      .SetMethod("addRepresentation", &NativeImage::AddRepresentation)
      // TODO(kevinsawicki): Remove in 2.0, deprecate before then with warnings
      .SetMethod("toPng", &NativeImage::ToPNG)
      .SetMethod("toJpeg", &NativeImage::ToJPEG);
}

}  // namespace api

}  // namespace atom

namespace mate {

v8::Local<v8::Value> Converter<mate::Handle<atom::api::NativeImage>>::ToV8(
    v8::Isolate* isolate,
    const mate::Handle<atom::api::NativeImage>& val) {
  return val.ToV8();
}

bool Converter<mate::Handle<atom::api::NativeImage>>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val,
    mate::Handle<atom::api::NativeImage>* out) {
  // Try converting from file path.
  base::FilePath path;
  if (ConvertFromV8(isolate, val, &path)) {
    *out = atom::api::NativeImage::CreateFromPath(isolate, path);
    // Should throw when failed to initialize from path.
    return !(*out)->image().IsEmpty();
  }

  WrappableBase* wrapper = static_cast<WrappableBase*>(internal::FromV8Impl(
      isolate, val));
  if (!wrapper)
    return false;

  *out = CreateHandle(isolate, static_cast<atom::api::NativeImage*>(wrapper));
  return true;
}

}  // namespace mate

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createEmpty", &atom::api::NativeImage::CreateEmpty);
  dict.SetMethod("createFromPath", &atom::api::NativeImage::CreateFromPath);
  dict.SetMethod("createFromBuffer", &atom::api::NativeImage::CreateFromBuffer);
  dict.SetMethod("createFromDataURL",
                 &atom::api::NativeImage::CreateFromDataURL);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_native_image, Initialize)
