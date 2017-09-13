// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_menu_views.h"

#include "atom/browser/native_window_views.h"
#include "atom/browser/unresponsive_suppressor.h"
#include "content/public/browser/render_widget_host_view.h"
#include "ui/display/screen.h"

using views::MenuRunner;

namespace atom {

namespace api {

MenuViews::MenuViews(v8::Isolate* isolate, v8::Local<v8::Object> wrapper)
    : Menu(isolate, wrapper),
      weak_factory_(this) {
}

void MenuViews::PopupAt(
    Window* window, int x, int y, int positioning_item, bool async) {
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

  int flags = MenuRunner::CONTEXT_MENU | MenuRunner::HAS_MNEMONICS;
  if (async)
    flags |= MenuRunner::ASYNC;

  // Don't emit unresponsive event when showing menu.
  atom::UnresponsiveSuppressor suppressor;

  // Show the menu.
  int32_t window_id = window->ID();
  auto close_callback = base::Bind(
      &MenuViews::ClosePopupAt, weak_factory_.GetWeakPtr(), window_id);
  menu_runners_[window_id] = std::unique_ptr<MenuRunner>(new MenuRunner(
      model(), flags, close_callback));
  menu_runners_[window_id]->RunMenuAt(
      static_cast<NativeWindowViews*>(window->window())->widget(),
      NULL,
      gfx::Rect(location, gfx::Size()),
      views::MENU_ANCHOR_TOPLEFT,
      ui::MENU_SOURCE_MOUSE);
}

void MenuViews::ClosePopupAt(int32_t window_id) {
  menu_runners_.erase(window_id);
}

// static
mate::WrappableBase* Menu::New(mate::Arguments* args) {
  return new MenuViews(args->isolate(), args->GetThis());
}

}  // namespace api

}  // namespace atom
