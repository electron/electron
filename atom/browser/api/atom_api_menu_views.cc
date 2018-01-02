// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_menu_views.h"

#include "atom/browser/native_window_views.h"
#include "atom/browser/unresponsive_suppressor.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "ui/display/screen.h"

using views::MenuRunner;

namespace atom {

namespace api {

MenuViews::MenuViews(v8::Isolate* isolate, v8::Local<v8::Object> wrapper)
    : Menu(isolate, wrapper),
      weak_factory_(this) {
}

void MenuViews::PopupAt(Window* window, int x, int y, int positioning_item,
                        const base::Closure& callback) {
  NativeWindow* native_window = static_cast<NativeWindow*>(window->window());
  if (!native_window)
    return;
  auto* web_contents = native_window->inspectable_web_contents();
  if (!web_contents)
    return;

  // (-1, -1) means showing on mouse location.
  gfx::Point location;
  if (x == -1 || y == -1) {
    location = display::Screen::GetScreen()->GetCursorScreenPoint();
  } else {
    auto* view = web_contents->GetView()->GetWebView();
    gfx::Point origin = view->bounds().origin();
    location = gfx::Point(origin.x() + x, origin.y() + y);
  }

  int flags = MenuRunner::CONTEXT_MENU | MenuRunner::HAS_MNEMONICS;

  // Don't emit unresponsive event when showing menu.
  atom::UnresponsiveSuppressor suppressor;

  // Show the menu.
  int32_t window_id = window->ID();
  auto close_callback = base::Bind(
      &MenuViews::OnClosed, weak_factory_.GetWeakPtr(), window_id, callback);
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
  auto runner = menu_runners_.find(window_id);
  if (runner != menu_runners_.end()) {
    // Close the runner for the window.
    runner->second->Cancel();
  } else if (window_id == -1) {
    // Or just close all opened runners.
    for (auto it = menu_runners_.begin(); it != menu_runners_.end();) {
      // The iterator is invalidated after the call.
      (it++)->second->Cancel();
    }
  }
}

void MenuViews::OnClosed(int32_t window_id, base::Closure callback) {
  menu_runners_.erase(window_id);
  callback.Run();
}

// static
mate::WrappableBase* Menu::New(mate::Arguments* args) {
  return new MenuViews(args->isolate(), args->GetThis());
}

}  // namespace api

}  // namespace atom
