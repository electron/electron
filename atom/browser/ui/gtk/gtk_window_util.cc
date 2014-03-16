// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/ui/gtk/gtk_window_util.h"

#include <dlfcn.h>
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"

using content::RenderWidgetHost;
using content::WebContents;

namespace gtk_window_util {

const int kFrameBorderThickness = 4;
const int kResizeAreaCornerSize = 16;

// Keep track of the last click time and the last click position so we can
// filter out extra GDK_BUTTON_PRESS events when a double click happens.
static guint32 last_click_time;
static int last_click_x;
static int last_click_y;

// Performs Cut/Copy/Paste operation on the |window|.
// If the current render view is focused, then just call the specified |method|
// against the current render view host, otherwise emit the specified |signal|
// against the focused widget.
// TODO(suzhe): This approach does not work for plugins.
void DoCutCopyPaste(GtkWindow* window,
                    WebContents* web_contents,
                    void (RenderWidgetHost::*method)(),
                    const char* signal) {
  GtkWidget* widget = gtk_window_get_focus(window);
  if (widget == NULL)
    return;  // Do nothing if no focused widget.

  if (web_contents &&
      widget == web_contents->GetView()->GetContentNativeView()) {
    (web_contents->GetRenderViewHost()->*method)();
  } else {
    guint id;
    if ((id = g_signal_lookup(signal, G_OBJECT_TYPE(widget))) != 0)
      g_signal_emit(widget, id, 0);
  }
}

void DoCut(GtkWindow* window, WebContents* web_contents) {
  DoCutCopyPaste(window, web_contents,
                 &RenderWidgetHost::Cut, "cut-clipboard");
}

void DoCopy(GtkWindow* window, WebContents* web_contents) {
  DoCutCopyPaste(window, web_contents,
                 &RenderWidgetHost::Copy, "copy-clipboard");
}

void DoPaste(GtkWindow* window, WebContents* web_contents) {
  DoCutCopyPaste(window, web_contents,
                 &RenderWidgetHost::Paste, "paste-clipboard");
}

// Ubuntu patches their version of GTK+ so that there is always a
// gripper in the bottom right corner of the window. We dynamically
// look up this symbol because it's a non-standard Ubuntu extension to
// GTK+. We always need to disable this feature since we can't
// communicate this to WebKit easily.
typedef void (*gtk_window_set_has_resize_grip_func)(GtkWindow*, gboolean);
gtk_window_set_has_resize_grip_func gtk_window_set_has_resize_grip_sym;

void DisableResizeGrip(GtkWindow* window) {
  static bool resize_grip_looked_up = false;
  if (!resize_grip_looked_up) {
    resize_grip_looked_up = true;
    gtk_window_set_has_resize_grip_sym =
        reinterpret_cast<gtk_window_set_has_resize_grip_func>(
            dlsym(NULL, "gtk_window_set_has_resize_grip"));
  }
  if (gtk_window_set_has_resize_grip_sym)
    gtk_window_set_has_resize_grip_sym(window, FALSE);
}

GdkCursorType GdkWindowEdgeToGdkCursorType(GdkWindowEdge edge) {
  switch (edge) {
    case GDK_WINDOW_EDGE_NORTH_WEST:
      return GDK_TOP_LEFT_CORNER;
    case GDK_WINDOW_EDGE_NORTH:
      return GDK_TOP_SIDE;
    case GDK_WINDOW_EDGE_NORTH_EAST:
      return GDK_TOP_RIGHT_CORNER;
    case GDK_WINDOW_EDGE_WEST:
      return GDK_LEFT_SIDE;
    case GDK_WINDOW_EDGE_EAST:
      return GDK_RIGHT_SIDE;
    case GDK_WINDOW_EDGE_SOUTH_WEST:
      return GDK_BOTTOM_LEFT_CORNER;
    case GDK_WINDOW_EDGE_SOUTH:
      return GDK_BOTTOM_SIDE;
    case GDK_WINDOW_EDGE_SOUTH_EAST:
      return GDK_BOTTOM_RIGHT_CORNER;
    default:
      NOTREACHED();
  }
  return GDK_LAST_CURSOR;
}

bool BoundsMatchMonitorSize(GtkWindow* window, gfx::Rect bounds) {
  // A screen can be composed of multiple monitors.
  GdkScreen* screen = gtk_window_get_screen(window);
  GdkRectangle monitor_size;

  if (gtk_widget_get_realized(GTK_WIDGET(window))) {
    // |window| has been realized.
    gint monitor_num = gdk_screen_get_monitor_at_window(screen,
        gtk_widget_get_window(GTK_WIDGET(window)));
    gdk_screen_get_monitor_geometry(screen, monitor_num, &monitor_size);
    return bounds.size() == gfx::Size(monitor_size.width, monitor_size.height);
  }

  // Make sure the window doesn't match any monitor size. We compare against
  // all monitors because we don't know which monitor the window is going to
  // open on before window realized.
  gint num_monitors = gdk_screen_get_n_monitors(screen);
  for (gint i = 0; i < num_monitors; ++i) {
    GdkRectangle monitor_size;
    gdk_screen_get_monitor_geometry(screen, i, &monitor_size);
    if (bounds.size() == gfx::Size(monitor_size.width, monitor_size.height))
      return true;
  }
  return false;
}

bool HandleTitleBarLeftMousePress(
    GtkWindow* window,
    const gfx::Rect& bounds,
    GdkEventButton* event) {
  // We want to start a move when the user single clicks, but not start a
  // move when the user double clicks.  However, a double click sends the
  // following GDK events: GDK_BUTTON_PRESS, GDK_BUTTON_RELEASE,
  // GDK_BUTTON_PRESS, GDK_2BUTTON_PRESS, GDK_BUTTON_RELEASE.  If we
  // start a gtk_window_begin_move_drag on the second GDK_BUTTON_PRESS,
  // the call to gtk_window_maximize fails.  To work around this, we
  // keep track of the last click and if it's going to be a double click,
  // we don't call gtk_window_begin_move_drag.
  DCHECK_EQ(event->type, GDK_BUTTON_PRESS);
  DCHECK_EQ(event->button, 1);

  static GtkSettings* settings = gtk_settings_get_default();
  gint double_click_time = 250;
  gint double_click_distance = 5;
  g_object_get(G_OBJECT(settings),
               "gtk-double-click-time", &double_click_time,
               "gtk-double-click-distance", &double_click_distance,
               NULL);

  guint32 click_time = event->time - last_click_time;
  int click_move_x = abs(event->x - last_click_x);
  int click_move_y = abs(event->y - last_click_y);

  last_click_time = event->time;
  last_click_x = static_cast<int>(event->x);
  last_click_y = static_cast<int>(event->y);

  if (click_time > static_cast<guint32>(double_click_time) ||
      click_move_x > double_click_distance ||
      click_move_y > double_click_distance) {
    // Ignore drag requests if the window is the size of the screen.
    // We do this to avoid triggering fullscreen mode in metacity
    // (without the --no-force-fullscreen flag) and in compiz (with
    // Legacy Fullscreen Mode enabled).
    if (!BoundsMatchMonitorSize(window, bounds)) {
      gtk_window_begin_move_drag(window, event->button,
                                 static_cast<gint>(event->x_root),
                                 static_cast<gint>(event->y_root),
                                 event->time);
    }
    return TRUE;
  }
  return FALSE;
}

void UnMaximize(GtkWindow* window,
                const gfx::Rect& bounds,
                const gfx::Rect& restored_bounds) {
  gtk_window_unmaximize(window);

  // It can happen that you end up with a window whose restore size is the same
  // as the size of the screen, so unmaximizing it merely remaximizes it due to
  // the same WM feature that SetWindowSize() works around.  We try to detect
  // this and resize the window to work around the issue.
  if (bounds.size() == restored_bounds.size())
    gtk_window_resize(window, bounds.width(), bounds.height() - 1);
}

void SetWindowCustomClass(GtkWindow* window, const std::string& wmclass) {
  gtk_window_set_wmclass(window,
                         wmclass.c_str(),
                         gdk_get_program_class());

  // Set WM_WINDOW_ROLE for session management purposes.
  // See http://tronche.com/gui/x/icccm/sec-5.html .
  gtk_window_set_role(window, wmclass.c_str());
}

void SetWindowSize(GtkWindow* window, const gfx::Size& size) {
  gfx::Size new_size = size;
  gint current_width = 0;
  gint current_height = 0;
  gtk_window_get_size(window, &current_width, &current_height);
  GdkRectangle size_with_decorations = {0};
  GdkWindow* gdk_window = gtk_widget_get_window(GTK_WIDGET(window));
  if (gdk_window) {
    gdk_window_get_frame_extents(gdk_window,
                                 &size_with_decorations);
  }

  if (current_width == size_with_decorations.width &&
      current_height == size_with_decorations.height) {
    // Make sure the window doesn't match any monitor size.  We compare against
    // all monitors because we don't know which monitor the window is going to
    // open on (the WM decides that).
    GdkScreen* screen = gtk_window_get_screen(window);
    gint num_monitors = gdk_screen_get_n_monitors(screen);
    for (gint i = 0; i < num_monitors; ++i) {
      GdkRectangle monitor_size;
      gdk_screen_get_monitor_geometry(screen, i, &monitor_size);
      if (gfx::Size(monitor_size.width, monitor_size.height) == size) {
        gtk_window_resize(window, size.width(), size.height() - 1);
        return;
      }
    }
  } else {
    // gtk_window_resize is the size of the window not including decorations,
    // but we are given the |size| including window decorations.
    if (size_with_decorations.width > current_width) {
      new_size.set_width(size.width() - size_with_decorations.width +
          current_width);
    }
    if (size_with_decorations.height > current_height) {
      new_size.set_height(size.height() - size_with_decorations.height +
          current_height);
    }
  }

  gtk_window_resize(window, new_size.width(), new_size.height());
}

bool GetWindowEdge(const gfx::Size& window_size,
                   int top_edge_inset,
                   int x,
                   int y,
                   GdkWindowEdge* edge) {
  gfx::Rect middle(window_size);
  middle.Inset(kFrameBorderThickness,
               kFrameBorderThickness - top_edge_inset,
               kFrameBorderThickness,
               kFrameBorderThickness);
  if (middle.Contains(x, y))
    return false;

  gfx::Rect north(0, 0, window_size.width(),
      kResizeAreaCornerSize - top_edge_inset);
  gfx::Rect west(0, 0, kResizeAreaCornerSize, window_size.height());
  gfx::Rect south(0, window_size.height() - kResizeAreaCornerSize,
      window_size.width(), kResizeAreaCornerSize);
  gfx::Rect east(window_size.width() - kResizeAreaCornerSize, 0,
      kResizeAreaCornerSize, window_size.height());

  if (north.Contains(x, y)) {
    if (west.Contains(x, y))
      *edge = GDK_WINDOW_EDGE_NORTH_WEST;
    else if (east.Contains(x, y))
      *edge = GDK_WINDOW_EDGE_NORTH_EAST;
    else
      *edge = GDK_WINDOW_EDGE_NORTH;
  } else if (south.Contains(x, y)) {
    if (west.Contains(x, y))
      *edge = GDK_WINDOW_EDGE_SOUTH_WEST;
    else if (east.Contains(x, y))
      *edge = GDK_WINDOW_EDGE_SOUTH_EAST;
    else
      *edge = GDK_WINDOW_EDGE_SOUTH;
  } else {
    if (west.Contains(x, y))
      *edge = GDK_WINDOW_EDGE_WEST;
    else if (east.Contains(x, y))
      *edge = GDK_WINDOW_EDGE_EAST;
    else
      return false;  // The cursor must be outside the window.
  }
  return true;
}

}  // namespace gtk_window_util
