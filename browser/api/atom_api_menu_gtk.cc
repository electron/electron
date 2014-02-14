// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_menu_gtk.h"

namespace atom {

namespace api {

MenuGtk::MenuGtk(v8::Handle<v8::Object> wrapper)
    : Menu(wrapper) {
}

MenuGtk::~MenuGtk() {
}

void MenuGtk::Popup(NativeWindow* native_window) {
}

// static
void Menu::AttachToWindow(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

// static
Menu* Menu::Create(v8::Handle<v8::Object> wrapper) {
  return new MenuGtk(wrapper);
}

}  // namespace api

}  // namespace atom
