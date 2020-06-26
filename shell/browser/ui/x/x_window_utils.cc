// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/x/x_window_utils.h"

#include <X11/Xatom.h>
#include <memory>

#include "base/environment.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/x/x11_atom_cache.h"

namespace electron {

void SetWMSpecState(x11::Window window, bool enabled, x11::Atom state) {
  XEvent xclient;
  memset(&xclient, 0, sizeof(xclient));
  xclient.type = ClientMessage;
  xclient.xclient.window = static_cast<uint32_t>(window);
  xclient.xclient.message_type =
      static_cast<uint32_t>(gfx::GetAtom("_NET_WM_STATE"));
  xclient.xclient.format = 32;
  xclient.xclient.data.l[0] = enabled ? 1 : 0;
  xclient.xclient.data.l[1] = static_cast<uint32_t>(state);
  xclient.xclient.data.l[2] = x11::None;
  xclient.xclient.data.l[3] = 1;
  xclient.xclient.data.l[4] = 0;

  XDisplay* xdisplay = gfx::GetXDisplay();
  XSendEvent(xdisplay, DefaultRootWindow(xdisplay), x11::False,
             SubstructureRedirectMask | SubstructureNotifyMask, &xclient);
}

void SetWindowType(x11::Window window, const std::string& type) {
  std::string type_prefix = "_NET_WM_WINDOW_TYPE_";
  x11::Atom window_type = gfx::GetAtom(type_prefix + base::ToUpperASCII(type));
  ui::SetProperty(window, gfx::GetAtom("_NET_WM_WINDOW_TYPE"), x11::Atom::ATOM,
                  window_type);
}

bool ShouldUseGlobalMenuBar() {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (env->HasVar("ELECTRON_FORCE_WINDOW_MENU_BAR"))
    return false;

  dbus::Bus::Options options;
  scoped_refptr<dbus::Bus> bus(new dbus::Bus(options));

  dbus::ObjectProxy* object_proxy =
      bus->GetObjectProxy(DBUS_SERVICE_DBUS, dbus::ObjectPath(DBUS_PATH_DBUS));
  dbus::MethodCall method_call(DBUS_INTERFACE_DBUS, "ListNames");
  std::unique_ptr<dbus::Response> response(object_proxy->CallMethodAndBlock(
      &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT));
  if (!response) {
    bus->ShutdownAndBlock();
    return false;
  }

  dbus::MessageReader reader(response.get());
  dbus::MessageReader array_reader(nullptr);
  if (!reader.PopArray(&array_reader)) {
    bus->ShutdownAndBlock();
    return false;
  }
  while (array_reader.HasMoreData()) {
    std::string name;
    if (array_reader.PopString(&name) &&
        name == "com.canonical.AppMenu.Registrar") {
      bus->ShutdownAndBlock();
      return true;
    }
  }

  bus->ShutdownAndBlock();
  return false;
}

void MoveWindowToForeground(x11::Window window) {
  MoveWindowAbove(window, static_cast<x11::Window>(0));
}

void MoveWindowAbove(x11::Window window, x11::Window other_window) {
  XDisplay* xdisplay = gfx::GetXDisplay();
  XEvent xclient;
  memset(&xclient, 0, sizeof(xclient));

  xclient.type = ClientMessage;
  xclient.xclient.display = xdisplay;
  xclient.xclient.window = static_cast<uint32_t>(window);
  xclient.xclient.message_type =
      static_cast<uint32_t>(gfx::GetAtom("_NET_RESTACK_WINDOW"));
  xclient.xclient.format = 32;
  xclient.xclient.data.l[0] = 2;
  xclient.xclient.data.l[1] = static_cast<uint32_t>(other_window);
  xclient.xclient.data.l[2] = static_cast<uint32_t>(x11::StackMode::Above);
  xclient.xclient.data.l[3] = 0;
  xclient.xclient.data.l[4] = 0;

  XSendEvent(xdisplay, DefaultRootWindow(xdisplay), x11::False,
             SubstructureRedirectMask | SubstructureNotifyMask, &xclient);
  XFlush(xdisplay);
}

bool IsWindowValid(x11::Window window) {
  XWindowAttributes attrs;
  return XGetWindowAttributes(gfx::GetXDisplay(), static_cast<uint32_t>(window),
                              &attrs);
}

}  // namespace electron
