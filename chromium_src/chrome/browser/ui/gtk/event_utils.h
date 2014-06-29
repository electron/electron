// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_GTK_EVENT_UTILS_H_
#define CHROME_BROWSER_UI_GTK_EVENT_UTILS_H_

#include <gtk/gtk.h>

#include "ui/base/window_open_disposition.h"

namespace event_utils {

// Translates event flags into plaform independent event flags.
int EventFlagsFromGdkState(guint state);

// Translates GdkEvent state into what kind of disposition they represent.
// For example, a middle click would mean to open a background tab.
WindowOpenDisposition DispositionFromGdkState(guint state);

// Get the window open disposition from the state in gtk_get_current_event().
// This is designed to be called inside a "clicked" event handler. It is an
// error to call it when gtk_get_current_event() won't return a GdkEventButton*.
WindowOpenDisposition DispositionForCurrentButtonPressEvent();

}  // namespace event_utils

#endif  // CHROME_BROWSER_UI_GTK_EVENT_UTILS_H_
