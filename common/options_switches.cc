// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/options_switches.h"

namespace atom {

namespace switches {

const char kTitle[]      = "title";
const char kToolbar[]    = "toolbar";
const char kIcon[]       = "icon";
const char kFrame[]      = "frame";
const char kShow[]       = "show";
const char kPosition[]   = "position";
const char kX[]          = "x";
const char kY[]          = "y";
const char kWidth[]      = "width";
const char kHeight[]     = "height";
const char kMinWidth[]   = "min_width";
const char kMinHeight[]  = "min_height";
const char kMaxWidth[]   = "max_width";
const char kMaxHeight[]  = "max_height";
const char kResizable[]  = "resizable";
const char kAsDesktop[]  = "as_desktop";
const char kFullscreen[] = "fullscreen";

// Start with the kiosk mode, see Opera's page for description:
// http://www.opera.com/support/mastering/kiosk/
const char kKiosk[] = "kiosk";

// Make windows stays on the top of all other windows.
const char kAlwaysOnTop[] = "always-on-top";

}  // namespace switches

}  // namespace atom
