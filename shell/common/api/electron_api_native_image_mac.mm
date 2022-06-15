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

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "gin/arguments.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/promise.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace electron {

namespace api {

NSData* bufferFromNSImage(NSImage* image) {
  CGImageRef ref = [image CGImageForProposedRect:nil context:nil hints:nil];
  NSBitmapImageRep* rep = [[NSBitmapImageRep alloc] initWithCGImage:ref];
  [rep setSize:[image size]];
  return [rep representationUsingType:NSPNGFileType
                           properties:[[NSDictionary alloc] init]];
}

double safeShift(double in, double def) {
  if (in >= 0 || in <= 1 || in == def)
    return in;
  return def;
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

  if (@available(macOS 10.15, *)) {
    NSURL* nsurl = base::mac::FilePathToNSURL(path);

    // We need to explicitly check if the user has passed an invalid path
    // because QLThumbnailGenerationRequest will generate a stock file icon
    // and pass silently if we do not.
    if (![[NSFileManager defaultManager] fileExistsAtPath:[nsurl path]]) {
      promise.RejectWithErrorMessage(
          "unable to retrieve thumbnail preview image for the given path");
      return handle;
    }

    NSScreen* screen = [[NSScreen screens] firstObject];
    base::scoped_nsobject<QLThumbnailGenerationRequest> request(
        [[QLThumbnailGenerationRequest alloc]
              initWithFileAtURL:nsurl
                           size:cg_size
                          scale:[screen backingScaleFactor]
            representationTypes:
                QLThumbnailGenerationRequestRepresentationTypeAll]);
    __block gin_helper::Promise<gfx::Image> p = std::move(promise);
    [[QLThumbnailGenerator sharedGenerator]
        generateBestRepresentationForRequest:request
                           completionHandler:^(
                               QLThumbnailRepresentation* thumbnail,
                               NSError* error) {
                             if (error || !thumbnail) {
                               std::string err_msg(
                                   [error.localizedDescription UTF8String]);
                               dispatch_async(dispatch_get_main_queue(), ^{
                                 p.RejectWithErrorMessage(
                                     "unable to retrieve thumbnail preview "
                                     "image for the given path: " +
                                     err_msg);
                               });
                             } else {
                               NSImage* result = [[[NSImage alloc]
                                   initWithCGImage:[thumbnail CGImage]
                                              size:cg_size] autorelease];
                               gfx::Image image(result);
                               dispatch_async(dispatch_get_main_queue(), ^{
                                 p.Resolve(image);
                               });
                             }
                           }];
  } else {
    base::ScopedCFTypeRef<CFURLRef> cfurl = base::mac::FilePathToCFURL(path);
    base::ScopedCFTypeRef<QLThumbnailRef> ql_thumbnail(
        QLThumbnailCreate(kCFAllocatorDefault, cfurl, cg_size, NULL));
    __block gin_helper::Promise<gfx::Image> p = std::move(promise);
    // Do not block the main thread waiting for quicklook to generate the
    // thumbnail.
    QLThumbnailDispatchAsync(
        ql_thumbnail,
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, /*flags*/ 0), ^{
          base::ScopedCFTypeRef<CGImageRef> cg_thumbnail(
              QLThumbnailCopyImage(ql_thumbnail));
          if (cg_thumbnail) {
            NSImage* result =
                [[[NSImage alloc] initWithCGImage:cg_thumbnail
                                             size:cg_size] autorelease];
            gfx::Image thumbnail(result);
            dispatch_async(dispatch_get_main_queue(), ^{
              p.Resolve(thumbnail);
            });
          } else {
            dispatch_async(dispatch_get_main_queue(), ^{
              p.RejectWithErrorMessage("unable to retrieve thumbnail preview "
                                       "image for the given path");
            });
          }
        });
  }

  return handle;
}

gin::Handle<NativeImage> NativeImage::CreateFromNamedImage(gin::Arguments* args,
                                                           std::string name) {
  @autoreleasepool {
    std::vector<double> hsl_shift;

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

    NSImage* image = [NSImage imageNamed:base::SysUTF8ToNSString(name)];

    if (!image.valid) {
      return CreateEmpty(args->isolate());
    }

    NSData* png_data = bufferFromNSImage(image);

    if (args->GetNext(&hsl_shift) && hsl_shift.size() == 3) {
      gfx::Image gfx_image = gfx::Image::CreateFrom1xPNGBytes(
          reinterpret_cast<const unsigned char*>((char*)[png_data bytes]),
          [png_data length]);
      color_utils::HSL shift = {safeShift(hsl_shift[0], -1),
                                safeShift(hsl_shift[1], 0.5),
                                safeShift(hsl_shift[2], 0.5)};
      png_data = bufferFromNSImage(
          gfx::Image(gfx::ImageSkiaOperations::CreateHSLShiftedImage(
                         gfx_image.AsImageSkia(), shift))
              .AsNSImage());
    }

    return CreateFromPNG(args->isolate(), (char*)[png_data bytes],
                         [png_data length]);
  }
}

void NativeImage::SetTemplateImage(bool setAsTemplate) {
  [image_.AsNSImage() setTemplate:setAsTemplate];
}

bool NativeImage::IsTemplateImage() {
  return [image_.AsNSImage() isTemplate];
}

}  // namespace api

}  // namespace electron
