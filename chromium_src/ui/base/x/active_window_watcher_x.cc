// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/x/active_window_watcher_x.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "base/memory/singleton.h"
#include "ui/base/x/active_window_watcher_x_observer.h"
#include "ui/base/x/root_window_property_watcher_x.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/gdk_compat.h"
#include "ui/gfx/gtk_compat.h"

namespace ui {

static const char* const kNetActiveWindow = "_NET_ACTIVE_WINDOW";

// static
ActiveWindowWatcherX* ActiveWindowWatcherX::GetInstance() {
  return Singleton<ActiveWindowWatcherX>::get();
}

// static
void ActiveWindowWatcherX::AddObserver(ActiveWindowWatcherXObserver* observer) {
  // Ensure that RootWindowPropertyWatcherX exists.
  internal::RootWindowPropertyWatcherX::GetInstance();
  GetInstance()->observers_.AddObserver(observer);
}

// static
void ActiveWindowWatcherX::RemoveObserver(
    ActiveWindowWatcherXObserver* observer) {
  GetInstance()->observers_.RemoveObserver(observer);
}

// static
Atom ActiveWindowWatcherX::GetPropertyAtom() {
  return GetAtom(kNetActiveWindow);
}

// static
void ActiveWindowWatcherX::Notify() {
  GetInstance()->NotifyActiveWindowChanged();
}

// static
bool ActiveWindowWatcherX::WMSupportsActivation() {
  return gdk_x11_screen_supports_net_wm_hint(
      gdk_screen_get_default(),
      gdk_atom_intern_static_string(kNetActiveWindow));
}

ActiveWindowWatcherX::ActiveWindowWatcherX() {
}

ActiveWindowWatcherX::~ActiveWindowWatcherX() {
}

void ActiveWindowWatcherX::NotifyActiveWindowChanged() {
  // We don't use gdk_screen_get_active_window() because it caches
  // whether or not the window manager supports _NET_ACTIVE_WINDOW.
  // This causes problems at startup for chromiumos.
  Atom type = None;
  int format = 0;  // size in bits of each item in 'property'
  long unsigned int num_items = 0, remaining_bytes = 0;
  unsigned char* property = NULL;

  XGetWindowProperty(gdk_x11_get_default_xdisplay(),
                     GDK_WINDOW_XID(gdk_get_default_root_window()),
                     GetAtom(kNetActiveWindow),
                     0,      // offset into property data to read
                     1,      // length to get in 32-bit quantities
                     False,  // deleted
                     AnyPropertyType,
                     &type,
                     &format,
                     &num_items,
                     &remaining_bytes,
                     &property);

  // Check that the property was set and contained a single 32-bit item (we
  // don't check that remaining_bytes is 0, though, as XFCE's window manager
  // seems to actually store two values in the property for some unknown
  // reason.)
  if (format == 32 && num_items == 1) {
    int xid = *reinterpret_cast<int*>(property);
    GdkDisplay* display = gdk_display_get_default();
    GdkWindow* active_window = gdk_x11_window_lookup_for_display(display, xid);
    FOR_EACH_OBSERVER(ActiveWindowWatcherXObserver, observers_,
                      ActiveWindowChanged(active_window));
  }
  if (property)
    XFree(property);
}

}  // namespace ui
