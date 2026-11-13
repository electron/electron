// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/global_menu_bar_registrar_x11.h"

#include <string>

#include "base/debug/leak_annotations.h"
#include "base/functional/bind.h"
#include "base/memory/singleton.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/linux/x11_util.h"
#include "shell/browser/ui/views/global_menu_bar_x11.h"
#include "ui/platform_window/extensions/wayland_extension.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_platform.h"

using content::BrowserThread;

namespace {

const char kAppMenuRegistrarName[] = "com.canonical.AppMenu.Registrar";
const char kAppMenuRegistrarPath[] = "/com/canonical/AppMenu/Registrar";

// Returns the Wayland toplevel extension of the window identified by |window|,
// or nullptr if the window no longer exists or isn't a Wayland toplevel.
ui::WaylandToplevelExtension* GetWaylandToplevelExtension(
    gfx::AcceleratedWidget window) {
  auto* host = views::DesktopWindowTreeHostPlatform::GetHostForWidget(window);
  if (!host || !host->platform_window())
    return nullptr;
  return ui::GetWaylandToplevelExtension(*host->platform_window());
}

}  // namespace

// static
GlobalMenuBarRegistrarX11* GlobalMenuBarRegistrarX11::GetInstance() {
  return base::Singleton<GlobalMenuBarRegistrarX11>::get();
}

void GlobalMenuBarRegistrarX11::OnWindowMapped(gfx::AcceleratedWidget window) {
  live_windows_.insert(window);

  if (registrar_proxy_)
    RegisterWindow(window);
}

void GlobalMenuBarRegistrarX11::OnWindowUnmapped(
    gfx::AcceleratedWidget window) {
  if (registrar_proxy_)
    UnregisterWindow(window);

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

void GlobalMenuBarRegistrarX11::RegisterWindow(gfx::AcceleratedWidget window) {
  if (x11_util::IsWayland())
    RegisterWaylandWindow(window);
  else
    RegisterXWindow(window);
}

void GlobalMenuBarRegistrarX11::UnregisterWindow(
    gfx::AcceleratedWidget window) {
  if (x11_util::IsWayland())
    UnregisterWaylandWindow(window);
  else
    UnregisterXWindow(window);
}

void GlobalMenuBarRegistrarX11::RegisterXWindow(gfx::AcceleratedWidget window) {
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

void GlobalMenuBarRegistrarX11::UnregisterXWindow(
    gfx::AcceleratedWidget window) {
  DCHECK(registrar_proxy_);

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

void GlobalMenuBarRegistrarX11::RegisterWaylandWindow(
    gfx::AcceleratedWidget window) {
  DCHECK(registrar_proxy_);
  auto* toplevel_extension = GetWaylandToplevelExtension(window);
  if (!toplevel_extension)
    return;

  // libdbusmenu exports the menu on the shared GIO session bus connection,
  // which is also the one |registrar_proxy_| was created on, so its unique
  // name is the service the compositor should look the menu up on.
  const char* service_name = g_dbus_connection_get_unique_name(
      g_dbus_proxy_get_connection(registrar_proxy_));
  if (!service_name)
    return;

  // The toplevel remembers this and re-announces it to the compositor
  // whenever its surface is recreated, e.g. when the window is shown again.
  toplevel_extension->SetAppmenu(
      service_name, electron::GlobalMenuBarX11::GetPathForWindow(window));
}

void GlobalMenuBarRegistrarX11::UnregisterWaylandWindow(
    gfx::AcceleratedWidget window) {
  if (auto* toplevel_extension = GetWaylandToplevelExtension(window))
    toplevel_extension->UnsetAppmenu();
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
  // If the name owner changed, we need to reregister all the live windows
  // with the system.
  for (const auto& window : live_windows_) {
    RegisterWindow(window);
  }
}
