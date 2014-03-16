// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/common/options_switches.h"

namespace atom {

namespace switches {

const char kTitle[]      = "title";
const char kIcon[]       = "icon";
const char kFrame[]      = "frame";
const char kShow[]       = "show";
const char kCenter[]     = "center";
const char kX[]          = "x";
const char kY[]          = "y";
const char kWidth[]      = "width";
const char kHeight[]     = "height";
const char kMinWidth[]   = "min-width";
const char kMinHeight[]  = "min-height";
const char kMaxWidth[]   = "max-width";
const char kMaxHeight[]  = "max-height";
const char kResizable[]  = "resizable";
const char kFullscreen[] = "fullscreen";

// Start with the kiosk mode, see Opera's page for description:
// http://www.opera.com/support/mastering/kiosk/
const char kKiosk[] = "kiosk";

// Make windows stays on the top of all other windows.
const char kAlwaysOnTop[] = "always-on-top";

const char kNodeIntegration[] = "node-integration";

// Enable the NSView to accept first mouse event.
const char kAcceptFirstMouse[] = "accept-first-mouse";

}  // namespace switches

}  // namespace atom
