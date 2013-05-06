// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "browser/api/atom_api_menu_mac.h"

namespace atom {

namespace api {

MenuMac::MenuMac(v8::Handle<v8::Object> wrapper)
    : Menu(wrapper),
      controller_([[MenuController alloc] initWithModel:model_.get()
                                 useWithPopUpButtonCell:NO]) {
}

MenuMac::~MenuMac() {
}

// static
Menu* Menu::Create(v8::Handle<v8::Object> wrapper) {
  return new MenuMac(wrapper);
}

}  // namespace api

}  // namespace atom
