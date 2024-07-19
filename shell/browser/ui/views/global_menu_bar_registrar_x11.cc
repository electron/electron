// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/global_menu_bar_registrar_x11.h"

#include <string>

#include "base/debug/leak_annotations.h"
#include "base/functional/bind.h"
#include "base/memory/singleton.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/ui/views/global_menu_bar_x11.h"

using content::BrowserThread;

namespace {

const char kAppMenuRegistrarName[] = "com.canonical.AppMenu.Registrar";
const char kAppMenuRegistrarPath[] = "/com/canonical/AppMenu/Registrar";

}  // namespace

// static
GlobalMenuBarRegistrarX11* GlobalMenuBarRegistrarX11::GetInstance() {
  return base::Singleton<GlobalMenuBarRegistrarX11>::get();
}

void GlobalMenuBarRegistrarX11::OnWindowMapped(x11::Window window) {
  live_windows_.insert(window);

  if (registrar_proxy_)
    RegisterXWindow(window);
}

void GlobalMenuBarRegistrarX11::OnWindowUnmapped(x11::Window window) {
  if (registrar_proxy_)
    UnregisterXWindow(window);

  live_windows_.erase(window);
}

GlobalMenuBarRegistrarX11::GlobalMenuBarRegistrarX11() {
  // libdbusmenu uses the gio version of dbus; I tried using the code in dbus/,
  // but it looks like that's isn't sharing the bus name with the gio version,
  // even when |connection_type| is set to SHARED.
  g_dbus_proxy_new_for_bus(
      G_BUS_TYPE_SESSION,
      static_cast<GDBusProxyFlags>(G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                   G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS |
                                   G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START),
      nullptr, kAppMenuRegistrarName, kAppMenuRegistrarPath,
      kAppMenuRegistrarName,
      nullptr,  // Probably want a real cancelable.
      static_cast<GAsyncReadyCallback>(OnProxyCreated), this);
}

GlobalMenuBarRegistrarX11::~GlobalMenuBarRegistrarX11() {
  if (registrar_proxy_) {
    g_object_unref(registrar_proxy_);
  }
}

void GlobalMenuBarRegistrarX11::RegisterXWindow(x11::Window window) {
  DCHECK(registrar_proxy_);
  std::string path = electron::GlobalMenuBarX11::GetPathForWindow(window);

  ANNOTATE_SCOPED_MEMORY_LEAK;  // http://crbug.com/314087
  // TODO(erg): The mozilla implementation goes to a lot of callback trouble
  // just to make sure that they react to make sure there's some sort of
  // cancelable object; including making a whole callback just to handle the
  // cancelable.
  //
  // I don't see any reason why we should care if "RegisterWindow" completes or
  // not.
  g_dbus_proxy_call(registrar_proxy_, "RegisterWindow",
                    g_variant_new("(uo)", window, path.c_str()),
                    G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
}

void GlobalMenuBarRegistrarX11::UnregisterXWindow(x11::Window window) {
  DCHECK(registrar_proxy_);
  std::string path = electron::GlobalMenuBarX11::GetPathForWindow(window);

  ANNOTATE_SCOPED_MEMORY_LEAK;  // http://crbug.com/314087
  // TODO(erg): The mozilla implementation goes to a lot of callback trouble
  // just to make sure that they react to make sure there's some sort of
  // cancelable object; including making a whole callback just to handle the
  // cancelable.
  //
  // I don't see any reason why we should care if "UnregisterWindow" completes
  // or not.
  g_dbus_proxy_call(registrar_proxy_, "UnregisterWindow",
                    g_variant_new("(u)", window), G_DBUS_CALL_FLAGS_NONE, -1,
                    nullptr, nullptr, nullptr);
}

void GlobalMenuBarRegistrarX11::OnProxyCreated(GObject* source,
                                               GAsyncResult* result,
                                               gpointer user_data) {
  GlobalMenuBarRegistrarX11* that =
      static_cast<GlobalMenuBarRegistrarX11*>(user_data);
  DCHECK(that);

  GError* error = nullptr;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish(result, &error);
  if (error) {
    g_error_free(error);
    return;
  }

  // TODO(erg): Mozilla's implementation has a workaround for GDBus
  // cancellation here. However, it's marked as fixed. If there's weird
  // problems with cancelation, look at how they fixed their issues.
  that->SetRegistrarProxy(proxy);

  that->OnNameOwnerChanged(nullptr, nullptr);
}

void GlobalMenuBarRegistrarX11::SetRegistrarProxy(GDBusProxy* proxy) {
  registrar_proxy_ = proxy;

  signal_ = ScopedGSignal(
      registrar_proxy_, "notify::g-name-owner",
      base::BindRepeating(&GlobalMenuBarRegistrarX11::OnNameOwnerChanged,
                          base::Unretained(this)));
}

void GlobalMenuBarRegistrarX11::OnNameOwnerChanged(GDBusProxy* /* ignored */,
                                                   GParamSpec* /* ignored */) {
  // If the name owner changed, we need to reregister all the live x11::Window
  // with the system.
  for (const auto& window : live_windows_) {
    RegisterXWindow(window);
  }
}
