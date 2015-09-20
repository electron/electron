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
  NativeWindow* native_window = static_cast<NativeWindow*>(window->window());
  if (!native_window)
    return;
  content::WebContents* web_contents = native_window->web_contents();
  if (!web_contents)
    return;
  content::RenderWidgetHostView* view = web_contents->GetRenderWidgetHostView();
  if (!view)
    return;

  gfx::Point origin = view->GetViewBounds().origin();
  PopupAtPoint(window, gfx::Point(origin.x() + x, origin.y() + y));
}

void MenuViews::PopupAtPoint(Window* window, const gfx::Point& point) {
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
