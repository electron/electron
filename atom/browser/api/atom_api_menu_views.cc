// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_menu_views.h"

#include "atom/browser/native_window_views.h"
#include "content/public/browser/render_widget_host_view.h"
#include "ui/gfx/screen.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace atom {

namespace api {

MenuViews::MenuViews() {
}

void MenuViews::Popup(Window* window) {
  PopupAtPoint(window, gfx::Screen::GetNativeScreen()->GetCursorScreenPoint());
}

void MenuViews::PopupAt(Window* window, int x, int y) {
  NativeWindow* nativeWindowViews =
    static_cast<NativeWindow*>(window->window());
  gfx::Point viewOrigin = nativeWindowViews->GetWebContents()
    ->GetRenderWidgetHostView()->GetViewBounds().origin();
  PopupAtPoint(window, gfx::Point(viewOrigin.x() + x, viewOrigin.y() + y));
}

void MenuViews::PopupAtPoint(Window* window, gfx::Point point) {
  views::MenuRunner menu_runner(
      model(),
      views::MenuRunner::CONTEXT_MENU | views::MenuRunner::HAS_MNEMONICS);
  ignore_result(menu_runner.RunMenuAt(
      static_cast<NativeWindowViews*>(window->window())->widget(),
      NULL,
      gfx::Rect(point, gfx::Size()),
      views::MENU_ANCHOR_TOPLEFT,
      ui::MENU_SOURCE_MOUSE));
}

// static
mate::Wrappable* Menu::Create() {
  return new MenuViews();
}

}  // namespace api

}  // namespace atom
