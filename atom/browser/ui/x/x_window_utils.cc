// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/x/x_window_utils.h"

#include <X11/Xatom.h>

#include "base/environment.h"
#include "base/strings/string_util.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"
#include "ui/base/x/x11_util.h"

namespace atom {

::Atom GetAtom(const char* name) {
  return XInternAtom(gfx::GetXDisplay(), name, false);
}

void SetWMSpecState(::Window xwindow, bool enabled, ::Atom state) {
  XEvent xclient;
  memset(&xclient, 0, sizeof(xclient));
  xclient.type = ClientMessage;
  xclient.xclient.window = xwindow;
  xclient.xclient.message_type = GetAtom("_NET_WM_STATE");
  xclient.xclient.format = 32;
  xclient.xclient.data.l[0] = enabled ? 1 : 0;
  xclient.xclient.data.l[1] = state;
  xclient.xclient.data.l[2] = None;
  xclient.xclient.data.l[3] = 1;
  xclient.xclient.data.l[4] = 0;

  XDisplay* xdisplay = gfx::GetXDisplay();
  XSendEvent(xdisplay, DefaultRootWindow(xdisplay), False,
             SubstructureRedirectMask | SubstructureNotifyMask,
             &xclient);
}

void SetWindowType(::Window xwindow, const std::string& type) {
  XDisplay* xdisplay = gfx::GetXDisplay();
  std::string type_prefix = "_NET_WM_WINDOW_TYPE_";
  ::Atom window_type = XInternAtom(
      xdisplay, (type_prefix + base::ToUpperASCII(type)).c_str(), False);
  XChangeProperty(xdisplay, xwindow,
                  XInternAtom(xdisplay, "_NET_WM_WINDOW_TYPE", False),
                  XA_ATOM,
                  32, PropModeReplace,
                  reinterpret_cast<unsigned char*>(&window_type), 1);
}

bool ShouldUseGlobalMenuBar() {
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
  dbus::MessageReader array_reader(NULL);
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

}  // namespace atom
