// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_menu_views.h"

#include "atom/browser/native_window_views.h"
#include "atom/browser/unresponsive_suppressor.h"
#include "content/public/browser/render_widget_host_view.h"
#include "ui/display/screen.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace atom {

namespace api {

MenuViews::MenuViews(v8::Isolate* isolate) : Menu(isolate) {
}

void MenuViews::PopupAt(Window* window, int x, int y, int positioning_item) {
  NativeWindow* native_window = static_cast<NativeWindow*>(window->window());
  if (!native_window)
    return;
  content::WebContents* web_contents = native_window->web_contents();
  if (!web_contents)
    return;
  content::RenderWidgetHostView* view = web_contents->GetRenderWidgetHostView();
  if (!view)
    return;

  // (-1, -1) means showing on mouse location.
  gfx::Point location;
  if (x == -1 || y == -1) {
    location = display::Screen::GetScreen()->GetCursorScreenPoint();
  } else {
    gfx::Point origin = view->GetViewBounds().origin();
    location = gfx::Point(origin.x() + x, origin.y() + y);
  }

  // Don't emit unresponsive event when showing menu.
  atom::UnresponsiveSuppressor suppressor;

  // Show the menu.
  views::MenuRunner menu_runner(
      model(),
      views::MenuRunner::CONTEXT_MENU | views::MenuRunner::HAS_MNEMONICS);
  ignore_result(menu_runner.RunMenuAt(
      static_cast<NativeWindowViews*>(window->window())->widget(),
      NULL,
      gfx::Rect(location, gfx::Size()),
      views::MENU_ANCHOR_TOPLEFT,
      ui::MENU_SOURCE_MOUSE));
}

// static
mate::WrappableBase* Menu::Create(v8::Isolate* isolate) {
  return new MenuViews(isolate);
}

}  // namespace api

}  // namespace atom
