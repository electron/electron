// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_X_X_WINDOW_UTILS_H_
#define ATOM_BROWSER_UI_X_X_WINDOW_UTILS_H_

#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>

namespace atom {

::Atom GetAtom(const char* name);

// Sends a message to the x11 window manager, enabling or disabling the |state|
// for _NET_WM_STATE.
void SetWMSpecState(::Window xwindow, bool enabled, ::Atom state);

}  // namespace atom

#endif  // ATOM_BROWSER_UI_X_X_WINDOW_UTILS_H_
