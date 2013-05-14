// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_MAC_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_MAC_H_

#include "browser/api/atom_api_menu.h"

namespace atom {

namespace api {

class MenuMac : public Menu {
 public:
  explicit MenuMac(v8::Handle<v8::Object> wrapper);
  virtual ~MenuMac();

 protected:
  virtual void Popup(NativeWindow* window) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(MenuMac);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_MAC_H_
