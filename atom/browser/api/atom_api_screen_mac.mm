// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#import "atom/browser/api/atom_api_screen.h"

namespace atom {

namespace api {

// TODO(codebytere): deprecated; remove in 3.0
int Screen::getMenuBarHeight() {
  return [[NSApp mainMenu] menuBarHeight];
}

}  // namespace api

}  // namespace atom
