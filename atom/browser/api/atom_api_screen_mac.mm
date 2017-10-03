// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/api/atom_api_screen.h"
#import <Cocoa/Cocoa.h>

namespace atom {

namespace api {

int Screen::getMenuBarHeight() {
  return [[NSApp mainMenu] menuBarHeight];
}

}// namespace api

}// namespace atom
