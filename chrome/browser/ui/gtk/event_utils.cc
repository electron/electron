// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/gtk/event_utils.h"

#include "base/logging.h"
#include "ui/base/window_open_disposition.h"
#include "ui/events/event_constants.h"

namespace event_utils {

int EventFlagsFromGdkState(guint state) {
  int flags = ui::EF_NONE;
  flags |= (state & GDK_LOCK_MASK) ? ui::EF_CAPS_LOCK_DOWN : ui::EF_NONE;
  flags |= (state & GDK_CONTROL_MASK) ? ui::EF_CONTROL_DOWN : ui::EF_NONE;
  flags |= (state & GDK_SHIFT_MASK) ? ui::EF_SHIFT_DOWN : ui::EF_NONE;
  flags |= (state & GDK_MOD1_MASK) ? ui::EF_ALT_DOWN : ui::EF_NONE;
  flags |= (state & GDK_BUTTON1_MASK) ? ui::EF_LEFT_MOUSE_BUTTON : ui::EF_NONE;
  flags |= (state & GDK_BUTTON2_MASK) ? ui::EF_MIDDLE_MOUSE_BUTTON
                                      : ui::EF_NONE;
  flags |= (state & GDK_BUTTON3_MASK) ? ui::EF_RIGHT_MOUSE_BUTTON : ui::EF_NONE;
  return flags;
}

// TODO(shinyak) This function will be removed after refactoring.
WindowOpenDisposition DispositionFromGdkState(guint state) {
  int event_flags = EventFlagsFromGdkState(state);
  return ui::DispositionFromEventFlags(event_flags);
}

WindowOpenDisposition DispositionForCurrentButtonPressEvent() {
  GdkEvent* event = gtk_get_current_event();
  if (!event) {
    NOTREACHED();
    return NEW_FOREGROUND_TAB;
  }

  guint state = event->button.state;
  gdk_event_free(event);
  return DispositionFromGdkState(state);
}

}  // namespace event_utils
