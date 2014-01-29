// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_menu_win.h"

#include "browser/native_window_win.h"
#include "browser/ui/win/menu_2.h"
#include "common/v8/native_type_conversions.h"
#include "ui/gfx/point.h"
#include "ui/gfx/screen.h"

#include "common/v8/node_common.h"

namespace atom {

namespace api {

MenuWin::MenuWin(v8::Handle<v8::Object> wrapper)
    : Menu(wrapper) {
}

MenuWin::~MenuWin() {
}

void MenuWin::Popup(NativeWindow* native_window) {
  gfx::Point cursor = gfx::Screen::GetNativeScreen()->GetCursorScreenPoint();
  menu_.reset(new atom::Menu2(model_.get()));
  menu_->RunContextMenuAt(cursor);
}

// static
void Menu::AttachToWindow(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Menu* self = ObjectWrap::Unwrap<Menu>(args.This());
  if (self == NULL)
    return node::ThrowError("Menu is already destroyed");

  NativeWindow* native_window;
  if (!FromV8Arguments(args, &native_window))
    return node::ThrowTypeError("Bad argument");

  static_cast<NativeWindowWin*>(native_window)->SetMenu(self->model_.get());
}

// static
Menu* Menu::Create(v8::Handle<v8::Object> wrapper) {
  return new MenuWin(wrapper);
}

}  // namespace api

}  // namespace atom
