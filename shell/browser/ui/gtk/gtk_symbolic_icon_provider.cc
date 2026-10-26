// Copyright (c) 2026 Mitchell Cohen.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/gtk/gtk_symbolic_icon_provider.h"

#include <dlfcn.h>
#include <gtk/gtk.h>

#include <string>

#include "electron/electron_gtk_stubs.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "ui/base/glib/scoped_gobject.h"

namespace electron {

// static
SymbolicIconProvider* GtkSymbolicIconProvider::GetInstance() {
  // Allow the app to fall back to another provider if GTK3 is not available.
  static void* const gtk3 = dlopen("libgtk-3.so.0", RTLD_LAZY | RTLD_NOLOAD);
  if (!gtk3 || !IsElectron_gdk_pixbufInitialized()) {
    return nullptr;
  }
  if (!IsElectron_gtkInitialized()) {
    InitializeElectron_gtk(gtk3);
    if (!IsElectron_gtkInitialized()) {
      return nullptr;
    }
  }
  static GtkSymbolicIconProvider instance;
  return &instance;
}

SkBitmap GtkSymbolicIconProvider::LoadIcon(const std::string& icon_name,
                                           int icon_size,
                                           int scale,
                                           SkColor color) {
  auto icon_info = TakeGObject(gtk_icon_theme_lookup_icon_for_scale(
      gtk_icon_theme_get_default(), icon_name.c_str(), icon_size, scale,
      static_cast<GtkIconLookupFlags>(GTK_ICON_LOOKUP_USE_BUILTIN |
                                      GTK_ICON_LOOKUP_GENERIC_FALLBACK)));
  if (!icon_info) {
    return {};
  }

  const GdkRGBA fg = {SkColorGetR(color) / 255.0, SkColorGetG(color) / 255.0,
                      SkColorGetB(color) / 255.0, SkColorGetA(color) / 255.0};
  auto pixbuf = TakeGObject(gtk_icon_info_load_symbolic(
      icon_info, &fg, nullptr, nullptr, nullptr, nullptr, nullptr));
  if (!pixbuf) {
    return {};
  }

  // Symbolic icons are decoded as unpremultiplied RGBA, so convert with Skia.
  SkBitmap bitmap;
  if (gdk_pixbuf_get_has_alpha(pixbuf)) {
    const SkImageInfo pixbuf_info = SkImageInfo::Make(
        gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf),
        kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
    bitmap.allocN32Pixels(pixbuf_info.width(), pixbuf_info.height());
    bitmap.writePixels(SkPixmap(pixbuf_info, gdk_pixbuf_get_pixels(pixbuf),
                                gdk_pixbuf_get_rowstride(pixbuf)));
  }
  return bitmap;
}

std::string GtkSymbolicIconProvider::GetThemeName() {
  GtkSettings* settings = gtk_settings_get_default();
  if (!settings) {
    return {};
  }
  gchar* value = nullptr;
  g_object_get(settings, "gtk-theme-name", &value, nullptr);
  std::string result = value ? value : "";
  g_free(value);
  return result;
}

}  // namespace electron
