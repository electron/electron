// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_

#include <gio/gio.h>

#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"
#include "ui/base/glib/scoped_gsignal.h"
#include "ui/gfx/x/xproto.h"

// Advertises our menu bars to desktops that consume the AppMenu D-Bus service.
//
// GlobalMenuBarX11 is responsible for managing the DbusmenuServer for each
// x11::Window. We need a separate object to own the dbus channel to
// com.canonical.AppMenu.Registrar and to register/unregister the mapping
// between a x11::Window and the DbusmenuServer instance we are offering.
class GlobalMenuBarRegistrarX11 {
 public:
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override = default;

    // |has_owner| is false when com.canonical.AppMenu.Registrar has no owner.
    virtual void OnMenuBarRegistrarOwnerChanged(bool has_owner) = 0;
  };

  static GlobalMenuBarRegistrarX11* GetInstance();

  void OnWindowMapped(x11::Window window);
  void OnWindowUnmapped(x11::Window window);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // disable copy
  GlobalMenuBarRegistrarX11(const GlobalMenuBarRegistrarX11&) = delete;
  GlobalMenuBarRegistrarX11& operator=(const GlobalMenuBarRegistrarX11&) =
      delete;

 private:
  friend struct base::DefaultSingletonTraits<GlobalMenuBarRegistrarX11>;

  GlobalMenuBarRegistrarX11();
  ~GlobalMenuBarRegistrarX11();

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
  absl::flat_hash_set<x11::Window> live_windows_;
  ScopedGSignal signal_;

  // Windows that exported a menu and need to show or hide their own menu bar
  // when the registrar appears or disappears.
  base::ObserverList<Observer,
                     false,
                     base::ObserverListReentrancyPolicy::kAllowReentrancy>
      observers_;
};

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_
