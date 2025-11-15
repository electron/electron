// Copyright (c) 2025 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "shell/browser/api/electron_api_tray.h"
#include "ui/gfx/image/image.h"

namespace electron::api {

// "Adaptive image" wraps two images (light and dark) and switches between them
//  based on the effective appearance of the context in which it is displayed.
static NSImage* MakeAdaptiveImage(NSImage* lightImage,
                                  NSImage* darkImage,
                                  NSSize iconSize) {
  NSImage* composite = [NSImage
       imageWithSize:iconSize
             flipped:NO
      drawingHandler:^BOOL(NSRect destRect) {
        NSAppearance* ap =
            NSAppearance.currentDrawingAppearance ?: NSApp.effectiveAppearance;
        BOOL isDark = [[[ap name] lowercaseString] containsString:@"dark"];

        NSImage* finalImg = isDark ? darkImage : lightImage;

        [finalImg drawInRect:destRect
                    fromRect:NSZeroRect
                   operation:NSCompositingOperationSourceOver
                    fraction:1.0];
        return YES;
      }];

  // The whole point of an adaptive image is to not use the template image mode.
  [composite setTemplate:NO];
  return composite;
}

// Renders a tinted version of the template, preserving various representations
// (1x/2x/3x).
static NSImage* TintTemplate(NSImage* templateImg,
                             NSColor* tint,
                             NSSize pointSize) {
  @autoreleasepool {
    NSImage* out = [[NSImage alloc] initWithSize:pointSize];

    // If there are no reps (odd), fallback to a single focus pass.
    NSImageRep* bestRep =
        [templateImg bestRepresentationForRect:(NSRect){.size = pointSize}
                                       context:nil
                                         hints:nil];
    NSArray<NSImageRep*>* reps = templateImg.representations.count
                                     ? templateImg.representations
                                     : (bestRep ? @[ bestRep ] : @[]);
    for (NSImageRep* rep in reps) {
      NSInteger pw = rep.pixelsWide;
      NSInteger ph = rep.pixelsHigh;
      if (pw <= 0 || ph <= 0) {
        continue;
      }

      NSBitmapImageRep* dst = [[NSBitmapImageRep alloc]
          initWithBitmapDataPlanes:NULL
                        pixelsWide:pw
                        pixelsHigh:ph
                     bitsPerSample:8
                   samplesPerPixel:4
                          hasAlpha:YES
                          isPlanar:NO
                    colorSpaceName:NSCalibratedRGBColorSpace
                      bitmapFormat:0
                       bytesPerRow:0
                      bitsPerPixel:0];

      NSGraphicsContext* ctx =
          [NSGraphicsContext graphicsContextWithBitmapImageRep:dst];
      [NSGraphicsContext saveGraphicsState];
      [NSGraphicsContext setCurrentContext:ctx];

      // Use no interpolation, because we're not doing any scaling.
      ctx.imageInterpolation = NSImageInterpolationNone;

      // Fill with tint, then keep alpha from the template via DestinationIn.
      [tint setFill];
      NSRectFill(NSMakeRect(0, 0, pw, ph));
      [templateImg drawInRect:NSMakeRect(0, 0, pw, ph)
                     fromRect:NSZeroRect
                    operation:NSCompositingOperationDestinationIn
                     fraction:1.0
               respectFlipped:NO
                        hints:@{
                          NSImageHintInterpolation : @(NSImageInterpolationNone)
                        }];
      [NSGraphicsContext restoreGraphicsState];

      // Attach the rep; NSImage uses point size to map pw/ph back to 1x/2x/3x.
      [out addRepresentation:dst];
    }
    return out;
  }
}

gfx::Image ComposeMultiLayerTrayImage(const std::vector<gfx::Image>& layers) {
  @autoreleasepool {
    if (layers.empty()) {
      return gfx::Image();
    }

    // Determine the maximum size across all layers
    NSSize iconSize = NSZeroSize;
    for (const auto& img : layers) {
      NSImage* nsImg = img.AsNSImage();
      if (nsImg) {
        if (nsImg.size.width > iconSize.width) {
          iconSize.width = nsImg.size.width;
        }
        if (nsImg.size.height > iconSize.height) {
          iconSize.height = nsImg.size.height;
        }
      }
    }

    if (iconSize.width <= 0 || iconSize.height <= 0) {
      return gfx::Image();
    }

    // Separate layers into template and non-template, tinting templates for
    // each mode.
    NSMutableArray<NSImage*>* lightLayers = [NSMutableArray array];
    NSMutableArray<NSImage*>* darkLayers = [NSMutableArray array];

    for (const auto& img : layers) {
      NSImage* nsImg = img.AsNSImage();
      if (!nsImg) {
        continue;
      }

      if ([nsImg isTemplate]) {
        // Template layers: manually tint to black/white at their original size.
        NSSize originalSize = nsImg.size;
        NSImage* lightVer =
            TintTemplate(nsImg, NSColor.blackColor, originalSize);
        NSImage* darkVer =
            TintTemplate(nsImg, NSColor.whiteColor, originalSize);
        [lightLayers addObject:lightVer];
        [darkLayers addObject:darkVer];
      } else {
        // Non-template layers: use as-is for both modes.
        [lightLayers addObject:nsImg];
        [darkLayers addObject:nsImg];
      }
    }

    // Helper to compose layers, preserving all representations (1x/2x/3x)
    auto composeLayers = ^NSImage*(NSArray<NSImage*>* layerArray) {
      NSImage* composite = [[NSImage alloc] initWithSize:iconSize];

      // Collect all unique scale factors from all layers.
      NSMutableSet<NSNumber*>* scales = [NSMutableSet set];
      for (NSImage* layer in layerArray) {
        for (NSImageRep* rep in layer.representations) {
          NSInteger pw = rep.pixelsWide;
          NSInteger ph = rep.pixelsHigh;
          if (pw > 0 && ph > 0) {
            CGFloat scale = (CGFloat)pw / layer.size.width;
            [scales addObject:@(scale)];
          }
        }
      }

      if (scales.count == 0) {
        [scales addObject:@(1.0)];
      }

      for (NSNumber* scaleNum in scales) {
        CGFloat scale = scaleNum.doubleValue;
        NSInteger pixelWidth = (NSInteger)(iconSize.width * scale);
        NSInteger pixelHeight = (NSInteger)(iconSize.height * scale);

        NSBitmapImageRep* bitmapRep = [[NSBitmapImageRep alloc]
            initWithBitmapDataPlanes:NULL
                          pixelsWide:pixelWidth
                          pixelsHigh:pixelHeight
                       bitsPerSample:8
                     samplesPerPixel:4
                            hasAlpha:YES
                            isPlanar:NO
                      colorSpaceName:NSCalibratedRGBColorSpace
                        bitmapFormat:NSBitmapFormatAlphaFirst
                         bytesPerRow:0
                        bitsPerPixel:0];

        NSGraphicsContext* ctx =
            [NSGraphicsContext graphicsContextWithBitmapImageRep:bitmapRep];
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:ctx];
        ctx.imageInterpolation = NSImageInterpolationNone;

        // Draw each layer at this scale.
        for (NSImage* layer in layerArray) {
          NSSize layerSize = layer.size;
          CGFloat layerPixelWidth = layerSize.width * scale;
          CGFloat layerPixelHeight = layerSize.height * scale;

          // Center the layer if it's smaller than the canvas
          CGFloat x = (pixelWidth - layerPixelWidth) / 2.0;
          CGFloat y = (pixelHeight - layerPixelHeight) / 2.0;
          NSRect destRect = NSMakeRect(x, y, layerPixelWidth, layerPixelHeight);

          [layer drawInRect:destRect
                    fromRect:NSZeroRect
                   operation:NSCompositingOperationSourceOver
                    fraction:1.0
              respectFlipped:NO
                       hints:@{
                         NSImageHintInterpolation : @(NSImageInterpolationNone)
                       }];
        }

        [NSGraphicsContext restoreGraphicsState];
        [composite addRepresentation:bitmapRep];
      }

      return composite;
    };

    // Pre-compose light and dark versions
    NSImage* lightComposite = composeLayers(lightLayers);
    NSImage* darkComposite = composeLayers(darkLayers);

    // Create adaptive image that switches between pre-rendered versions
    NSImage* composite =
        MakeAdaptiveImage(lightComposite, darkComposite, iconSize);
    return gfx::Image(composite);
  }
}

}  // namespace electron::api
