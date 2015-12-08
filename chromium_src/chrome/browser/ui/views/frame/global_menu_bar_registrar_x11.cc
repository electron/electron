// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/global_menu_bar_registrar_x11.h"

#include "atom/browser/ui/views/global_menu_bar_x11.h"
#include "base/bind.h"
#include "base/debug/leak_annotations.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace {

const char kAppMenuRegistrarName[] = "com.canonical.AppMenu.Registrar";
const char kAppMenuRegistrarPath[] = "/com/canonical/AppMenu/Registrar";

}  // namespace

// static
GlobalMenuBarRegistrarX11* GlobalMenuBarRegistrarX11::GetInstance() {
  return base::Singleton<GlobalMenuBarRegistrarX11>::get();
}

void GlobalMenuBarRegistrarX11::OnWindowMapped(unsigned long xid) {
  live_xids_.insert(xid);

  if (registrar_proxy_)
    RegisterXID(xid);
}

void GlobalMenuBarRegistrarX11::OnWindowUnmapped(unsigned long xid) {
  if (registrar_proxy_)
    UnregisterXID(xid);

  live_xids_.erase(xid);
}

GlobalMenuBarRegistrarX11::GlobalMenuBarRegistrarX11()
    : registrar_proxy_(nullptr) {
  // libdbusmenu uses the gio version of dbus; I tried using the code in dbus/,
  // but it looks like that's isn't sharing the bus name with the gio version,
  // even when |connection_type| is set to SHARED.
  g_dbus_proxy_new_for_bus(
      G_BUS_TYPE_SESSION,
      static_cast<GDBusProxyFlags>(
          G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
          G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS |
          G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START),
      nullptr,
      kAppMenuRegistrarName,
      kAppMenuRegistrarPath,
      kAppMenuRegistrarName,
      nullptr,  // TODO: Probalby want a real cancelable.
      static_cast<GAsyncReadyCallback>(OnProxyCreatedThunk),
      this);
}

GlobalMenuBarRegistrarX11::~GlobalMenuBarRegistrarX11() {
  if (registrar_proxy_) {
    g_signal_handlers_disconnect_by_func(
        registrar_proxy_,
        reinterpret_cast<void*>(OnNameOwnerChangedThunk),
        this);
    g_object_unref(registrar_proxy_);
  }
}

void GlobalMenuBarRegistrarX11::RegisterXID(unsigned long xid) {
  DCHECK(registrar_proxy_);
  std::string path = atom::GlobalMenuBarX11::GetPathForWindow(xid);

  ANNOTATE_SCOPED_MEMORY_LEAK; // http://crbug.com/314087
  // TODO(erg): The mozilla implementation goes to a lot of callback trouble
  // just to make sure that they react to make sure there's some sort of
  // cancelable object; including making a whole callback just to handle the
  // cancelable.
  //
  // I don't see any reason why we should care if "RegisterWindow" completes or
  // not.
  g_dbus_proxy_call(registrar_proxy_,
                    "RegisterWindow",
                    g_variant_new("(uo)", xid, path.c_str()),
                    G_DBUS_CALL_FLAGS_NONE, -1,
                    nullptr,
                    nullptr,
                    nullptr);
}

void GlobalMenuBarRegistrarX11::UnregisterXID(unsigned long xid) {
  DCHECK(registrar_proxy_);
  std::string path = atom::GlobalMenuBarX11::GetPathForWindow(xid);

  ANNOTATE_SCOPED_MEMORY_LEAK; // http://crbug.com/314087
  // TODO(erg): The mozilla implementation goes to a lot of callback trouble
  // just to make sure that they react to make sure there's some sort of
  // cancelable object; including making a whole callback just to handle the
  // cancelable.
  //
  // I don't see any reason why we should care if "UnregisterWindow" completes
  // or not.
  g_dbus_proxy_call(registrar_proxy_,
                    "UnregisterWindow",
                    g_variant_new("(u)", xid),
                    G_DBUS_CALL_FLAGS_NONE, -1,
                    nullptr,
                    nullptr,
                    nullptr);
}

void GlobalMenuBarRegistrarX11::OnProxyCreated(GObject* source,
                                               GAsyncResult* result) {
  GError* error = nullptr;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish(result, &error);
  if (error) {
    g_error_free(error);
    return;
  }

  // TODO(erg): Mozilla's implementation has a workaround for GDBus
  // cancellation here. However, it's marked as fixed. If there's weird
  // problems with cancelation, look at how they fixed their issues.

  registrar_proxy_ = proxy;

  g_signal_connect(registrar_proxy_, "notify::g-name-owner",
                   G_CALLBACK(OnNameOwnerChangedThunk), this);

  OnNameOwnerChanged(nullptr, nullptr);
}

void GlobalMenuBarRegistrarX11::OnNameOwnerChanged(GObject* /* ignored */,
                                                   GParamSpec* /* ignored */) {
  // If the name owner changed, we need to reregister all the live xids with
  // the system.
  for (std::set<unsigned long>::const_iterator it = live_xids_.begin();
       it != live_xids_.end(); ++it) {
    RegisterXID(*it);
  }
}
