// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_

#include <gio/gio.h>

#include <set>

#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "ui/base/glib/scoped_gsignal.h"
#include "ui/gfx/x/xproto.h"

// Advertises our menu bars to Unity.
//
// GlobalMenuBarX11 is responsible for managing the DbusmenuServer for each
// x11::Window. We need a separate object to own the dbus channel to
// com.canonical.AppMenu.Registrar and to register/unregister the mapping
// between a x11::Window and the DbusmenuServer instance we are offering.
class GlobalMenuBarRegistrarX11 {
 public:
  static GlobalMenuBarRegistrarX11* GetInstance();

  void OnWindowMapped(x11::Window window);
  void OnWindowUnmapped(x11::Window window);

 private:
  friend struct base::DefaultSingletonTraits<GlobalMenuBarRegistrarX11>;

  GlobalMenuBarRegistrarX11();
  ~GlobalMenuBarRegistrarX11();

  // disable copy
  GlobalMenuBarRegistrarX11(const GlobalMenuBarRegistrarX11&) = delete;
  GlobalMenuBarRegistrarX11& operator=(const GlobalMenuBarRegistrarX11&) =
      delete;

  // Sends the actual message.
  void RegisterXWindow(x11::Window window);
  void UnregisterXWindow(x11::Window window);

  static void OnProxyCreated(GObject* source,
                             GAsyncResult* result,
                             gpointer user_data);
  void OnNameOwnerChanged(GDBusProxy* /* ignored */, GParamSpec* /* ignored */);
  void SetRegistrarProxy(GDBusProxy* proxy);

  raw_ptr<GDBusProxy> registrar_proxy_ = nullptr;

  // x11::Window which want to be registered, but haven't yet been because
  // we're waiting for the proxy to become available.
  std::set<x11::Window> live_windows_;
  ScopedGSignal signal_;
};

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_
