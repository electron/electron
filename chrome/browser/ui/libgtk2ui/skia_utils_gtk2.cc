// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtk2ui/skia_utils_gtk2.h"

#include <gdk/gdk.h>

#include "base/basictypes.h"
#include "base/logging.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkUnPreMultiply.h"

namespace libgtk2ui {

// GDK_COLOR_RGB multiplies by 257 (= 0x10001) to distribute the bits evenly
// See: http://www.mindcontrol.org/~hplus/graphics/expand-bits.html
// To get back, we can just right shift by eight
// (or, formulated differently, i == (i*257)/256 for all i < 256).

SkColor GdkColorToSkColor(GdkColor color) {
  return SkColorSetRGB(color.red >> 8, color.green >> 8, color.blue >> 8);
}

GdkColor SkColorToGdkColor(SkColor color) {
  GdkColor gdk_color = {
      0,
      static_cast<guint16>(SkColorGetR(color) * kSkiaToGDKMultiplier),
      static_cast<guint16>(SkColorGetG(color) * kSkiaToGDKMultiplier),
      static_cast<guint16>(SkColorGetB(color) * kSkiaToGDKMultiplier)
  };
  return gdk_color;
}

const SkBitmap GdkPixbufToImageSkia(GdkPixbuf* pixbuf) {
  // TODO(erg): What do we do in the case where the pixbuf fails these dchecks?
  // I would prefer to use our gtk based canvas, but that would require
  // recompiling half of our skia extensions with gtk support, which we can't
  // do in this build.
  DCHECK_EQ(GDK_COLORSPACE_RGB, gdk_pixbuf_get_colorspace(pixbuf));

  int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
  int w = gdk_pixbuf_get_width(pixbuf);
  int h = gdk_pixbuf_get_height(pixbuf);

  SkBitmap ret;
  ret.setConfig(SkBitmap::kARGB_8888_Config, w, h);
  ret.allocPixels();
  ret.eraseColor(0);

  uint32_t* skia_data = static_cast<uint32_t*>(ret.getAddr(0, 0));

  if (n_channels == 4) {
    int total_length = w * h;
    guchar* gdk_pixels = gdk_pixbuf_get_pixels(pixbuf);

    // Now here's the trick: we need to convert the gdk data (which is RGBA and
    // isn't premultiplied) to skia (which can be anything and premultiplied).
    for (int i = 0; i < total_length; ++i, gdk_pixels += 4) {
      const unsigned char& red = gdk_pixels[0];
      const unsigned char& green = gdk_pixels[1];
      const unsigned char& blue = gdk_pixels[2];
      const unsigned char& alpha = gdk_pixels[3];

      skia_data[i] = SkPreMultiplyARGB(alpha, red, green, blue);
    }
  } else if (n_channels == 3) {
    // Because GDK makes rowstrides word aligned, we need to do something a bit
    // more complex when a pixel isn't perfectly a word of memory.
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar* gdk_pixels = gdk_pixbuf_get_pixels(pixbuf);
    for (int y = 0; y < h; ++y) {
      int row = y * rowstride;

      for (int x = 0; x < w; ++x) {
        guchar* pixel = gdk_pixels + row + (x * 3);
        const unsigned char& red = pixel[0];
        const unsigned char& green = pixel[1];
        const unsigned char& blue = pixel[2];

        skia_data[y * w + x] = SkPreMultiplyARGB(255, red, green, blue);
      }
    }
  } else {
    NOTREACHED();
  }

  return ret;
}

GdkPixbuf* GdkPixbufFromSkBitmap(const SkBitmap& bitmap) {
  if (bitmap.isNull())
    return NULL;

  SkAutoLockPixels lock_pixels(bitmap);

  int width = bitmap.width();
  int height = bitmap.height();

  GdkPixbuf* pixbuf =
      gdk_pixbuf_new(GDK_COLORSPACE_RGB,  // The only colorspace gtk supports.
                     TRUE,                // There is an alpha channel.
                     8,
                     width,
                     height);

  // SkBitmaps are premultiplied, we need to unpremultiply them.
  const int kBytesPerPixel = 4;
  uint8* divided = gdk_pixbuf_get_pixels(pixbuf);

  for (int y = 0, i = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint32 pixel = bitmap.getAddr32(0, y)[x];

      int alpha = SkColorGetA(pixel);
      if (alpha != 0 && alpha != 255) {
        SkColor unmultiplied = SkUnPreMultiply::PMColorToColor(pixel);
        divided[i + 0] = SkColorGetR(unmultiplied);
        divided[i + 1] = SkColorGetG(unmultiplied);
        divided[i + 2] = SkColorGetB(unmultiplied);
        divided[i + 3] = alpha;
      } else {
        divided[i + 0] = SkColorGetR(pixel);
        divided[i + 1] = SkColorGetG(pixel);
        divided[i + 2] = SkColorGetB(pixel);
        divided[i + 3] = alpha;
      }
      i += kBytesPerPixel;
    }
  }

  return pixbuf;
}

}  // namespace libgtk2ui
