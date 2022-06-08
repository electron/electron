// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/gtk_util.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdint.h>

#include <string>

#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkUnPreMultiply.h"
#include "ui/gtk/gtk_compat.h"  // nogncheck

// The following utilities are pulled from
// https://source.chromium.org/chromium/chromium/src/+/main:ui/gtk/select_file_dialog_impl_gtk.cc;l=43-74
namespace gtk_util {

namespace {

const char* GettextPackage() {
  static base::NoDestructor<std::string> gettext_package(
      "gtk" + base::NumberToString(gtk::GtkVersion().components()[0]) + "0");
  return gettext_package->c_str();
}

const char* GtkGettext(const char* str) {
  return g_dgettext(GettextPackage(), str);
}

}  // namespace

const char* GetCancelLabel() {
  static const char* cancel = GtkGettext("_Cancel");
  return cancel;
}

const char* GetOpenLabel() {
  static const char* open = GtkGettext("_Open");
  return open;
}

const char* GetSaveLabel() {
  static const char* save = GtkGettext("_Save");
  return save;
}

const char* GetOkLabel() {
  static const char* ok = GtkGettext("_Ok");
  return ok;
}

const char* GetNoLabel() {
  static const char* no = GtkGettext("_No");
  return no;
}

const char* GetYesLabel() {
  static const char* yes = GtkGettext("_Yes");
  return yes;
}

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
