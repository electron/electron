// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_GTK_GTK_WINDOW_UTIL_H_
#define CHROME_BROWSER_UI_GTK_GTK_WINDOW_UTIL_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "ui/gfx/rect.h"

namespace content {
class WebContents;
}

namespace gtk_window_util {

// The frame border is only visible in restored mode and is hardcoded to 4 px
// on each side regardless of the system window border size.
extern const int kFrameBorderThickness;
// In the window corners, the resize areas don't actually expand bigger, but
// the 16 px at the end of each edge triggers diagonal resizing.
extern const int kResizeAreaCornerSize;

// Ubuntu patches their version of GTK+ to that there is always a
// gripper in the bottom right corner of the window. We always need to
// disable this feature since we can't communicate this to WebKit easily.
void DisableResizeGrip(GtkWindow* window);

// Returns the resize cursor corresponding to the window |edge|.
GdkCursorType GdkWindowEdgeToGdkCursorType(GdkWindowEdge edge);

// Returns |true| if the window bounds match the monitor size.
bool BoundsMatchMonitorSize(GtkWindow* window, gfx::Rect bounds);

bool HandleTitleBarLeftMousePress(GtkWindow* window,
                                  const gfx::Rect& bounds,
                                  GdkEventButton* event);

// Request the underlying window to unmaximize.  Also tries to work around
// a window manager "feature" that can prevent this in some edge cases.
void UnMaximize(GtkWindow* window,
                const gfx::Rect& bounds,
                const gfx::Rect& restored_bounds);

// Set a custom WM_CLASS for a window.
void SetWindowCustomClass(GtkWindow* window, const std::string& wmclass);

// A helper method for setting the GtkWindow size that should be used in place
// of calling gtk_window_resize directly.  This is done to avoid a WM "feature"
// where setting the window size to the monitor size causes the WM to set the
// EWMH for full screen mode.
void SetWindowSize(GtkWindow* window, const gfx::Size& size);

// If the point (|x|, |y|) is within the resize border area of the window,
// returns true and sets |edge| to the appropriate GdkWindowEdge value.
// Otherwise, returns false.
// |top_edge_inset| specifies how much smaller (in px) than the default edge
// size the top edge should be, used by browser windows to make it easier to
// move the window since a lot of title bar space is taken by the tabs.
bool GetWindowEdge(const gfx::Size& window_size,
                   int top_edge_inset,
                   int x,
                   int y,
                   GdkWindowEdge* edge);

}  // namespace gtk_window_util

#endif  // CHROME_BROWSER_UI_GTK_GTK_WINDOW_UTIL_H_
