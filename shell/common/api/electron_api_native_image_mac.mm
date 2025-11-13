// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_native_image.h"

#include <string>
#include <utility>
#include <vector>

#import <Cocoa/Cocoa.h>
#import <QuickLook/QuickLook.h>
#import <QuickLookThumbnailing/QuickLookThumbnailing.h>

#include "base/apple/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/values.h"
#include "gin/arguments.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/mac_util.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace electron::api {

NSData* bufferFromNSImage(NSImage* image) {
  CGImageRef ref = [image CGImageForProposedRect:nil context:nil hints:nil];
  NSBitmapImageRep* rep = [[NSBitmapImageRep alloc] initWithCGImage:ref];
  [rep setSize:[image size]];
  return [rep representationUsingType:NSBitmapImageFileTypePNG
                           properties:[[NSDictionary alloc] init]];
}

double safeShift(double in, double def) {
  if ((in >= 0 && in <= 1) || in == def)
    return in;
  return def;
}

void ReceivedThumbnailResult(CGSize size,
                             gin_helper::Promise<gfx::Image> p,
                             QLThumbnailRepresentation* thumbnail,
                             NSError* error) {
  if (error || !thumbnail) {
    std::string err_msg([error.localizedDescription UTF8String]);
    p.RejectWithErrorMessage("unable to retrieve thumbnail preview "
                             "image for the given path: " +
                             err_msg);
  } else {
    NSImage* result = [[NSImage alloc] initWithCGImage:[thumbnail CGImage]
                                                  size:size];
    gfx::Image image(result);
    p.Resolve(image);
  }
}

// static
v8::Local<v8::Promise> NativeImage::CreateThumbnailFromPath(
    v8::Isolate* isolate,
    const base::FilePath& path,
    const gfx::Size& size) {
  gin_helper::Promise<gfx::Image> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (size.IsEmpty()) {
    promise.RejectWithErrorMessage("size must not be empty");
    return handle;
  }

  CGSize cg_size = size.ToCGSize();

  NSURL* nsurl = base::apple::FilePathToNSURL(path);

  // We need to explicitly check if the user has passed an invalid path
  // because QLThumbnailGenerationRequest will generate a stock file icon
  // and pass silently if we do not.
  if (![[NSFileManager defaultManager] fileExistsAtPath:[nsurl path]]) {
    promise.RejectWithErrorMessage(
        "unable to retrieve thumbnail preview image for the given path");
    return handle;
  }

  NSScreen* screen = [[NSScreen screens] firstObject];
  QLThumbnailGenerationRequest* request([[QLThumbnailGenerationRequest alloc]
        initWithFileAtURL:nsurl
                     size:cg_size
                    scale:[screen backingScaleFactor]
      representationTypes:QLThumbnailGenerationRequestRepresentationTypeAll]);
  __block auto block_callback = base::BindPostTaskToCurrentDefault(
      base::BindOnce(&ReceivedThumbnailResult, cg_size, std::move(promise)));
  auto completionHandler =
      ^(QLThumbnailRepresentation* thumbnail, NSError* error) {
        std::move(block_callback).Run(thumbnail, error);
      };
  [[QLThumbnailGenerator sharedGenerator]
      generateBestRepresentationForRequest:request
                         completionHandler:completionHandler];

  return handle;
}

gin_helper::Handle<NativeImage> NativeImage::CreateMenuSymbol(
    gin::Arguments* args,
    std::string name) {
  @autoreleasepool {
    float pointSize = 13.0;
    NSFontWeight weight = NSFontWeightSemibold;
    NSImageSymbolScale scale = NSImageSymbolScaleSmall;

    NSImage* image =
        [NSImage imageWithSystemSymbolName:base::SysUTF8ToNSString(name)
                  accessibilityDescription:nil];

    NSImageSymbolConfiguration* symbol_config =
        [NSImageSymbolConfiguration configurationWithPointSize:pointSize
                                                        weight:weight
                                                         scale:scale];

    image = [image imageWithSymbolConfiguration:symbol_config];

    NSData* png_data = bufferFromNSImage(image);

    gin_helper::Handle<NativeImage> handle =
        CreateFromPNG(args->isolate(), electron::util::as_byte_span(png_data));

    gfx::Size size = handle->GetSize(1.0);

    float aspect_ratio =
        static_cast<float>(size.width()) / static_cast<float>(size.height());

    int new_width = 14;
    int new_height = static_cast<int>(14 / aspect_ratio);

    // prevent tall symbols from exceeding menu item height (e.g. chevron.right)
    if (new_height >= 16) {
      new_height = 14;
      new_width = static_cast<int>(14 * aspect_ratio);
    }

    gin_helper::Handle<NativeImage> sized = handle->Resize(
        args,
        base::Value::Dict().Set("width", new_width).Set("height", new_height));

    sized->SetTemplateImage(true);

    return sized;
  }
}

gin_helper::Handle<NativeImage> NativeImage::CreateFromNamedImage(
    gin::Arguments* args,
    std::string name) {
  @autoreleasepool {
    std::vector<double> hsl_shift;

    float pointSize = 30.0;
    NSFontWeight weight = NSFontWeightRegular;
    NSImageSymbolScale scale = NSImageSymbolScaleMedium;

    args->GetNext(&hsl_shift);

    gin_helper::Dictionary options;
    if (args->GetNext(&options)) {
      options.Get("pointSize", &pointSize);

      std::string weight_str;
      options.Get("weight", &weight_str);

      // We unfortunately have to map from string to NSFontWeight manually, as
      // NSFontWeight maps to a double under the hood.
      if (weight_str == "ultralight") {
        weight = NSFontWeightUltraLight;
      } else if (weight_str == "thin") {
        weight = NSFontWeightThin;
      } else if (weight_str == "light") {
        weight = NSFontWeightLight;
      } else if (weight_str == "regular") {
        weight = NSFontWeightRegular;
      } else if (weight_str == "medium") {
        weight = NSFontWeightMedium;
      } else if (weight_str == "semibold") {
        weight = NSFontWeightSemibold;
      } else if (weight_str == "bold") {
        weight = NSFontWeightBold;
      } else if (weight_str == "heavy") {
        weight = NSFontWeightHeavy;
      } else if (weight_str == "black") {
        weight = NSFontWeightBlack;
      }

      std::string scale_str;
      options.Get("scale", &scale_str);

      // Similarly, map from string to NSImageSymbolScale.
      if (scale_str == "small") {
        scale = NSImageSymbolScaleSmall;
      } else if (scale_str == "medium") {
        scale = NSImageSymbolScaleMedium;
      } else if (scale_str == "large") {
        scale = NSImageSymbolScaleLarge;
      }
    }

    // The string representations of NSImageNames don't match the strings
    // themselves; they instead follow the following pattern:
    //  * NSImageNameActionTemplate -> "NSActionTemplate"
    //  * NSImageNameMultipleDocuments -> "NSMultipleDocuments"
    // To account for this, we strip out "ImageName" from the passed string.
    std::string to_remove("ImageName");
    size_t pos = name.find(to_remove);
    if (pos != std::string::npos) {
      name.erase(pos, to_remove.length());
    }

    NSImage* image = nil;
    NSString* ns_name = base::SysUTF8ToNSString(name);

    // Treat non-Cocoa-prefixed names as SF Symbols first.
    if (!base::StartsWith(name, "NS") && !base::StartsWith(name, "NX")) {
      image = [NSImage imageWithSystemSymbolName:ns_name
                        accessibilityDescription:nil];

      NSImageSymbolConfiguration* symbol_config =
          [NSImageSymbolConfiguration configurationWithPointSize:pointSize
                                                          weight:weight
                                                           scale:scale];

      image = [image imageWithSymbolConfiguration:symbol_config];
    } else {
      image = [NSImage imageNamed:ns_name];
    }

    if (!image || !image.valid) {
      return CreateEmpty(args->isolate());
    }

    NSData* png_data = bufferFromNSImage(image);

    if (hsl_shift.size() == 3) {
      auto gfx_image = gfx::Image::CreateFrom1xPNGBytes(
          electron::util::as_byte_span(png_data));
      color_utils::HSL shift = {safeShift(hsl_shift[0], -1),
                                safeShift(hsl_shift[1], 0.5),
                                safeShift(hsl_shift[2], 0.5)};
      png_data = bufferFromNSImage(
          gfx::Image(gfx::ImageSkiaOperations::CreateHSLShiftedImage(
                         gfx_image.AsImageSkia(), shift))
              .AsNSImage());
    }

    return CreateFromPNG(args->isolate(),
                         electron::util::as_byte_span(png_data));
  }
}

void NativeImage::SetTemplateImage(bool setAsTemplate) {
  [image_.AsNSImage() setTemplate:setAsTemplate];
}

bool NativeImage::IsTemplateImage() {
  return [image_.AsNSImage() isTemplate];
}

}  // namespace electron::api
