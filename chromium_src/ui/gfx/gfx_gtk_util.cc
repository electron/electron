// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gtk_util.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#include "base/basictypes.h"
#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkUnPreMultiply.h"
#include "ui/gfx/rect.h"

namespace {

// A process wide singleton that manages our usage of gdk cursors.
// gdk_cursor_new() hits the disk in several places and GdkCursor instances can
// be reused throughout the process.
class GdkCursorCache {
 public:
  GdkCursorCache() {}
  ~GdkCursorCache() {
    for (GdkCursorMap::iterator i(cursors_.begin()); i != cursors_.end(); ++i) {
      gdk_cursor_unref(i->second);
    }
    cursors_.clear();
  }

  GdkCursor* GetCursorImpl(GdkCursorType type) {
    GdkCursorMap::iterator it = cursors_.find(type);
    GdkCursor* cursor = NULL;
    if (it == cursors_.end()) {
      cursor = gdk_cursor_new(type);
      cursors_.insert(std::make_pair(type, cursor));
    } else {
      cursor = it->second;
    }

    // It is not necessary to add a reference here. The callers can ref the
    // cursor if they need it for something.
    return cursor;
  }

 private:
  typedef std::map<GdkCursorType, GdkCursor*> GdkCursorMap;
  GdkCursorMap cursors_;

  DISALLOW_COPY_AND_ASSIGN(GdkCursorCache);
};

}  // namespace

namespace gfx {

static void CommonInitFromCommandLine(const CommandLine& command_line,
                                      void (*init_func)(gint*, gchar***)) {
  const std::vector<std::string>& args = command_line.argv();
  int argc = args.size();
  scoped_ptr<char *[]> argv(new char *[argc + 1]);
  for (size_t i = 0; i < args.size(); ++i) {
    // TODO(piman@google.com): can gtk_init modify argv? Just being safe
    // here.
    argv[i] = strdup(args[i].c_str());
  }
  argv[argc] = NULL;
  char **argv_pointer = argv.get();

  init_func(&argc, &argv_pointer);
  for (size_t i = 0; i < args.size(); ++i) {
    free(argv[i]);
  }
}

void GtkInitFromCommandLine(const CommandLine& command_line) {
  CommonInitFromCommandLine(command_line, gtk_init);
}

void GdkInitFromCommandLine(const CommandLine& command_line) {
  CommonInitFromCommandLine(command_line, gdk_init);
}

GdkPixbuf* GdkPixbufFromSkBitmap(const SkBitmap& bitmap) {
  if (bitmap.isNull())
    return NULL;

  SkAutoLockPixels lock_pixels(bitmap);

  int width = bitmap.width();
  int height = bitmap.height();

  GdkPixbuf* pixbuf = gdk_pixbuf_new(
      GDK_COLORSPACE_RGB,  // The only colorspace gtk supports.
      TRUE,  // There is an alpha channel.
      8,
      width, height);

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

void SubtractRectanglesFromRegion(GdkRegion* region,
                                  const std::vector<Rect>& cutouts) {
  for (size_t i = 0; i < cutouts.size(); ++i) {
    GdkRectangle rect = {
        cutouts[i].x(), cutouts[i].y(), cutouts[i].width(), cutouts[i].height()
    };
    GdkRegion* rect_region = gdk_region_rectangle(&rect);
    gdk_region_subtract(region, rect_region);
    // TODO(deanm): It would be nice to be able to reuse the GdkRegion here.
    gdk_region_destroy(rect_region);
  }
}

GdkCursor* GetCursor(int type) {
  CR_DEFINE_STATIC_LOCAL(GdkCursorCache, impl, ());
  return impl.GetCursorImpl(static_cast<GdkCursorType>(type));
}

void InitRCStyles() {
  static const char kRCText[] =
      // Make our dialogs styled like the GNOME HIG.
      //
      // TODO(evanm): content-area-spacing was introduced in a later
      // version of GTK, so we need to set that manually on all dialogs.
      // Perhaps it would make sense to have a shared FixupDialog() function.
      "style \"gnome-dialog\" {\n"
      "  xthickness = 12\n"
      "  GtkDialog::action-area-border = 0\n"
      "  GtkDialog::button-spacing = 6\n"
      "  GtkDialog::content-area-spacing = 18\n"
      "  GtkDialog::content-area-border = 12\n"
      "}\n"
      // Note we set it at the "application" priority, so users can override.
      "widget \"GtkDialog\" style : application \"gnome-dialog\"\n"

      // Make our about dialog special, so the image is flush with the edge.
      "style \"about-dialog\" {\n"
      "  GtkDialog::action-area-border = 12\n"
      "  GtkDialog::button-spacing = 6\n"
      "  GtkDialog::content-area-spacing = 18\n"
      "  GtkDialog::content-area-border = 0\n"
      "}\n"
      "widget \"about-dialog\" style : application \"about-dialog\"\n";

  gtk_rc_parse_string(kRCText);
}

base::TimeDelta GetCursorBlinkCycle() {
  // From http://library.gnome.org/devel/gtk/unstable/GtkSettings.html, this is
  // the default value for gtk-cursor-blink-time.
  static const gint kGtkDefaultCursorBlinkTime = 1200;

  gint cursor_blink_time = kGtkDefaultCursorBlinkTime;
  gboolean cursor_blink = TRUE;
  g_object_get(gtk_settings_get_default(),
               "gtk-cursor-blink-time", &cursor_blink_time,
               "gtk-cursor-blink", &cursor_blink,
               NULL);
  return cursor_blink ?
         base::TimeDelta::FromMilliseconds(cursor_blink_time) :
         base::TimeDelta();
}

}  // namespace gfx
