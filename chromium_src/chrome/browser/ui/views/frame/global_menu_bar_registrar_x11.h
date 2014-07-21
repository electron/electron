// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_GLOBAL_MENU_BAR_REGISTRAR_X11_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_GLOBAL_MENU_BAR_REGISTRAR_X11_H_

#include <gio/gio.h>

#include <set>

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "ui/base/glib/glib_signal.h"

// Advertises our menu bars to Unity.
//
// GlobalMenuBarX11 is responsible for managing the DbusmenuServer for each
// XID. We need a separate object to own the dbus channel to
// com.canonical.AppMenu.Registrar and to register/unregister the mapping
// between a XID and the DbusmenuServer instance we are offering.
class GlobalMenuBarRegistrarX11 {
 public:
  static GlobalMenuBarRegistrarX11* GetInstance();

  void OnWindowMapped(unsigned long xid);
  void OnWindowUnmapped(unsigned long xid);

 private:
  friend struct DefaultSingletonTraits<GlobalMenuBarRegistrarX11>;

  GlobalMenuBarRegistrarX11();
  ~GlobalMenuBarRegistrarX11();

  // Sends the actual message.
  void RegisterXID(unsigned long xid);
  void UnregisterXID(unsigned long xid);

  CHROMEG_CALLBACK_1(GlobalMenuBarRegistrarX11, void, OnProxyCreated,
                     GObject*, GAsyncResult*);
  CHROMEG_CALLBACK_1(GlobalMenuBarRegistrarX11, void, OnNameOwnerChanged,
                     GObject*, GParamSpec*);

  GDBusProxy* registrar_proxy_;

  // Window XIDs which want to be registered, but haven't yet been because
  // we're waiting for the proxy to become available.
  std::set<unsigned long> live_xids_;

  DISALLOW_COPY_AND_ASSIGN(GlobalMenuBarRegistrarX11);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_GLOBAL_MENU_BAR_REGISTRAR_X11_H_
