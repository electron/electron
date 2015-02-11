// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/x/x_window_utils.h"

#include <X11/Xatom.h>

#include "base/strings/string_util.h"
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
      xdisplay, (type_prefix + StringToUpperASCII(type)).c_str(), False);
  XChangeProperty(xdisplay, xwindow,
                  XInternAtom(xdisplay, "_NET_WM_WINDOW_TYPE", False),
                  XA_ATOM,
                  32, PropModeReplace,
                  reinterpret_cast<unsigned char*>(&window_type), 1);
}

}  // namespace atom
