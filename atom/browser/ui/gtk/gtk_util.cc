// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/ui/gtk/gtk_util.h"

#include <cairo/cairo.h>

#include "base/logging.h"

namespace gtk_util {

namespace {

const char kBoldLabelMarkup[] = "<span weight='bold'>%s</span>";

// Returns the approximate number of characters that can horizontally fit in
// |pixel_width| pixels.
int GetCharacterWidthForPixels(GtkWidget* widget, int pixel_width) {
  DCHECK(gtk_widget_get_realized(widget))
      << " widget must be realized to compute font metrics correctly";

  PangoContext* context = gtk_widget_create_pango_context(widget);
  GtkStyle* style = gtk_widget_get_style(widget);
  PangoFontMetrics* metrics = pango_context_get_metrics(context,
      style->font_desc, pango_context_get_language(context));

  // This technique (max of char and digit widths) matches the code in
  // gtklabel.c.
  int char_width = pixel_width * PANGO_SCALE /
      std::max(pango_font_metrics_get_approximate_char_width(metrics),
               pango_font_metrics_get_approximate_digit_width(metrics));

  pango_font_metrics_unref(metrics);
  g_object_unref(context);

  return char_width;
}

void OnLabelRealize(GtkWidget* label, gpointer pixel_width) {
  gtk_label_set_width_chars(
      GTK_LABEL(label),
      GetCharacterWidthForPixels(label, GPOINTER_TO_INT(pixel_width)));
}

}  // namespace

GtkWidget* LeftAlignMisc(GtkWidget* misc) {
  gtk_misc_set_alignment(GTK_MISC(misc), 0, 0.5);
  return misc;
}

GtkWidget* CreateBoldLabel(const std::string& text) {
  GtkWidget* label = gtk_label_new(NULL);
  char* markup = g_markup_printf_escaped(kBoldLabelMarkup, text.c_str());
  gtk_label_set_markup(GTK_LABEL(label), markup);
  g_free(markup);

  return LeftAlignMisc(label);
}

void SetAlwaysShowImage(GtkWidget* image_menu_item) {
  gtk_image_menu_item_set_always_show_image(
      GTK_IMAGE_MENU_ITEM(image_menu_item), TRUE);
}

bool IsWidgetAncestryVisible(GtkWidget* widget) {
  GtkWidget* parent = widget;
  while (parent && gtk_widget_get_visible(parent))
    parent = gtk_widget_get_parent(parent);
  return !parent;
}

void SetLabelWidth(GtkWidget* label, int pixel_width) {
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

  // Do the simple thing in LTR because the bug only affects right-aligned
  // text. Also, when using the workaround, the label tries to maintain
  // uniform line-length, which we don't really want.
  if (gtk_widget_get_direction(label) == GTK_TEXT_DIR_LTR) {
    gtk_widget_set_size_request(label, pixel_width, -1);
  } else {
    // The label has to be realized before we can adjust its width.
    if (gtk_widget_get_realized(label)) {
      OnLabelRealize(label, GINT_TO_POINTER(pixel_width));
    } else {
      g_signal_connect(label, "realize", G_CALLBACK(OnLabelRealize),
                       GINT_TO_POINTER(pixel_width));
    }
  }
}

}  // namespace gtk_util
