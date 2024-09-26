// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/gtk_util.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "electron/electron_gtk_stubs.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkUnPreMultiply.h"
#include "ui/gtk/gtk_compat.h"  // nogncheck

// The following utilities are pulled from
// https://source.chromium.org/chromium/chromium/src/+/main:ui/gtk/select_file_dialog_linux_gtk.cc;l=44-75;drc=a03ba4ca94f75531207c3ea832d6a605cde77394
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
    return {};

  constexpr int kBytesPerPixel = 4;
  const auto [width, height] = bitmap.dimensions();
  std::vector<uint8_t> bytes;
  bytes.reserve(width * height * kBytesPerPixel);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const SkColor pixel = bitmap.getColor(x, y);
      bytes.emplace_back(SkColorGetR(pixel));
      bytes.emplace_back(SkColorGetG(pixel));
      bytes.emplace_back(SkColorGetB(pixel));
      bytes.emplace_back(SkColorGetA(pixel));
    }
  }

  constexpr GdkColorspace kColorspace = GDK_COLORSPACE_RGB;
  constexpr gboolean kHasAlpha = true;
  constexpr int kBitsPerSample = 8;
  return gdk_pixbuf_new_from_bytes(
      g_bytes_new(std::data(bytes), std::size(bytes)), kColorspace, kHasAlpha,
      kBitsPerSample, width, height,
      gdk_pixbuf_calculate_rowstride(kColorspace, kHasAlpha, kBitsPerSample,
                                     width, height));
}

}  // namespace gtk_util
