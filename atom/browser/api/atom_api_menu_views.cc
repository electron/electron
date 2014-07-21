// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_menu_views.h"

#include "atom/browser/native_window_views.h"
#include "ui/gfx/screen.h"
#include "ui/views/controls/menu/menu_runner.h"

#if defined(OS_WIN)
#include "ui/gfx/win/dpi.h"
#endif

namespace atom {

namespace api {

MenuViews::MenuViews() {
}

void MenuViews::Popup(Window* window) {
  gfx::Point cursor = gfx::Screen::GetNativeScreen()->GetCursorScreenPoint();
#if defined(OS_WIN)
  cursor = gfx::win::ScreenToDIPPoint(cursor);
#endif

  views::MenuRunner menu_runner(model());
  ignore_result(menu_runner.RunMenuAt(
      static_cast<NativeWindowViews*>(window->window())->widget(),
      NULL,
      gfx::Rect(cursor, gfx::Size()),
      views::MenuItemView::TOPLEFT,
      ui::MENU_SOURCE_MOUSE,
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU));
}

// static
mate::Wrappable* Menu::Create() {
  return new MenuViews();
}

}  // namespace api

}  // namespace atom
