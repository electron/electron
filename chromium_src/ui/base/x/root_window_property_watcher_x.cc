// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/x/root_window_property_watcher_x.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "base/memory/singleton.h"
#include "ui/base/x/active_window_watcher_x.h"
#include "ui/base/x/work_area_watcher_x.h"

namespace ui {

namespace internal {

// static
RootWindowPropertyWatcherX* RootWindowPropertyWatcherX::GetInstance() {
  return Singleton<RootWindowPropertyWatcherX>::get();
}

RootWindowPropertyWatcherX::RootWindowPropertyWatcherX() {
  GdkWindow* root = gdk_get_default_root_window();

  // Set up X Event filter to listen for PropertyChange X events.
  // Don't use XSelectInput directly here, as gdk internally seems to cache the
  // mask and reapply XSelectInput after this, resetting any mask we set here.
  gdk_window_set_events(root,
                        static_cast<GdkEventMask>(gdk_window_get_events(root) |
                                                  GDK_PROPERTY_CHANGE_MASK));
  gdk_window_add_filter(root,
                        &RootWindowPropertyWatcherX::OnWindowXEventThunk,
                        this);
}

RootWindowPropertyWatcherX::~RootWindowPropertyWatcherX() {
  gdk_window_remove_filter(NULL,
                           &RootWindowPropertyWatcherX::OnWindowXEventThunk,
                           this);
}

GdkFilterReturn RootWindowPropertyWatcherX::OnWindowXEvent(
    GdkXEvent* xevent, GdkEvent* event) {
  XEvent* xev = static_cast<XEvent*>(xevent);

  if (xev->xany.type == PropertyNotify) {
    if (xev->xproperty.atom == ActiveWindowWatcherX::GetPropertyAtom())
      ActiveWindowWatcherX::Notify();
    else if (xev->xproperty.atom == WorkAreaWatcherX::GetPropertyAtom())
      WorkAreaWatcherX::Notify();
  }

  return GDK_FILTER_CONTINUE;
}

}  // namespace internal

}  // namespace ui
