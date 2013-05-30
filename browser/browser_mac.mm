// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/browser.h"

#import "browser/atom_application_mac.h"

namespace atom {

void Browser::Terminate() {
  [[AtomApplication sharedApplication] terminate:nil];
}

void Browser::Focus() {
  [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
}

}  // namespace atom
