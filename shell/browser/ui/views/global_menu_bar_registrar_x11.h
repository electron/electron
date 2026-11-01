// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_

#include <gio/gio.h>

#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"
#include "ui/base/glib/scoped_gsignal.h"
#include "ui/gfx/native_ui_types.h"

// Advertises our menu bars to desktops that consume the AppMenu D-Bus service.
//
// GlobalMenuBarX11 is responsible for managing the DbusmenuServer for each
// window. We need a separate object to own the dbus channel to
// com.canonical.AppMenu.Registrar and to register/unregister the mapping
// between a window and the DbusmenuServer instance we are offering.
//
// On X11 the mapping is registered with com.canonical.AppMenu.Registrar using
// the X window id. On Wayland, where there are no X window ids, it is set on
// the window's toplevel surface instead via the org_kde_kwin_appmenu protocol
// (see ui::WaylandToplevelExtension::SetAppmenu()). Either way we only
// advertise menus while the registrar is present, matching Chromium.
class GlobalMenuBarRegistrarX11 {
 public:
  static GlobalMenuBarRegistrarX11* GetInstance();

  void OnWindowMapped(gfx::AcceleratedWidget window);
  void OnWindowUnmapped(gfx::AcceleratedWidget window);

  // disable copy
  GlobalMenuBarRegistrarX11(const GlobalMenuBarRegistrarX11&) = delete;
  GlobalMenuBarRegistrarX11& operator=(const GlobalMenuBarRegistrarX11&) =
      delete;

 private:
  friend struct base::DefaultSingletonTraits<GlobalMenuBarRegistrarX11>;

  GlobalMenuBarRegistrarX11();
  ~GlobalMenuBarRegistrarX11();

  // Advertises (or stops advertising) the menu for |window| using whichever
  // mechanism the current platform supports.
  void RegisterWindow(gfx::AcceleratedWidget window);
  void UnregisterWindow(gfx::AcceleratedWidget window);

  // Sends the actual message.
  void RegisterXWindow(gfx::AcceleratedWidget window);
  void UnregisterXWindow(gfx::AcceleratedWidget window);

  // Associates the menu with the window's Wayland toplevel. These look the
  // window up by |window| on every call rather than holding on to it, since
  // the window can be destroyed before its menu bar is.
  void RegisterWaylandWindow(gfx::AcceleratedWidget window);
  void UnregisterWaylandWindow(gfx::AcceleratedWidget window);

  static void OnProxyCreated(GObject* source,
                             GAsyncResult* result,
                             gpointer user_data);
  void OnNameOwnerChanged(GDBusProxy* /* ignored */, GParamSpec* /* ignored */);
  void SetRegistrarProxy(GDBusProxy* proxy);

  raw_ptr<GDBusProxy> registrar_proxy_ = nullptr;

  // Windows which want to be registered, but haven't yet been because
  // we're waiting for the proxy to become available.
  absl::flat_hash_set<gfx::AcceleratedWidget> live_windows_;
  ScopedGSignal signal_;
};

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_
