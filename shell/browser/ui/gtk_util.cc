// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/gtk_util.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdint.h>

#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkUnPreMultiply.h"

namespace gtk_util {

// Copied from L40-L55 in
// https://cs.chromium.org/chromium/src/chrome/browser/ui/libgtkui/select_file_dialog_impl_gtk.cc
#if GTK_CHECK_VERSION(3, 90, 0)
// GTK stock items have been deprecated.  The docs say to switch to using the
// strings "_Open", etc.  However this breaks i18n.  We could supply our own
// internationalized strings, but the "_" in these strings is significant: it's
// the keyboard shortcut to select these actions.  TODO: Provide
// internationalized strings when GTK provides support for it.
const char* const kCancelLabel = "_Cancel";
const char* const kNoLabel = "_No";
const char* const kOkLabel = "_OK";
const char* const kOpenLabel = "_Open";
const char* const kSaveLabel = "_Save";
const char* const kYesLabel = "_Yes";
#else
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
const char* const kCancelLabel = GTK_STOCK_CANCEL;
const char* const kNoLabel = GTK_STOCK_NO;
const char* const kOkLabel = GTK_STOCK_OK;
const char* const kOpenLabel = GTK_STOCK_OPEN;
const char* const kSaveLabel = GTK_STOCK_SAVE;
const char* const kYesLabel = GTK_STOCK_YES;
G_GNUC_END_IGNORE_DEPRECATIONS
#endif

GdkPixbuf* GdkPixbufFromSkBitmap(const SkBitmap& bitmap) {
  if (bitmap.isNull())
    return nullptr;

  int width = bitmap.width();
  int height = bitmap.height();

  GdkPixbuf* pixbuf =
      gdk_pixbuf_new(GDK_COLORSPACE_RGB,  // The only colorspace gtk supports.
                     TRUE,                // There is an alpha channel.
                     8, width, height);

  // SkBitmaps are premultiplied, we need to unpremultiply them.
  const int kBytesPerPixel = 4;
  uint8_t* divided = gdk_pixbuf_get_pixels(pixbuf);

  for (int y = 0, i = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint32_t pixel = bitmap.getAddr32(0, y)[x];

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

}  // namespace gtk_util
