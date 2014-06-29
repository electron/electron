// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines utility functions for X11 (Linux only). This code has been
// ported from XCB since we can't use XCB on Ubuntu while its 32-bit support
// remains woefully incomplete.

#include "ui/base/x/x11_util.h"

#include "ui/gfx/gdk_compat.h"

namespace ui {

Atom GetAtom(const char* name) {
#if defined(TOOLKIT_GTK)
  return gdk_x11_get_xatom_by_name_for_display(
      gdk_display_get_default(), name);
#else
  // TODO(derat): Cache atoms to avoid round-trips to the server.
  return XInternAtom(gfx::GetXDisplay(), name, false);
#endif
}

}  // namespace ui
