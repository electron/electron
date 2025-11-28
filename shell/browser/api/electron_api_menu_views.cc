// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_menu_views.h"

#include <memory>
#include <utility>

#include "shell/browser/api/electron_api_base_window.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/browser/native_window_views.h"
#include "shell/common/callback_util.h"
#include "ui/display/screen.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"

using views::MenuRunner;

namespace electron::api {

MenuViews::MenuViews(gin::Arguments* args) : Menu(args) {}

MenuViews::~MenuViews() = default;

void MenuViews::PopupAt(BaseWindow* window,
                        std::optional<WebFrameMain*> frame,
                        int x,
                        int y,
                        int positioning_item,
                        ui::mojom::MenuSourceType source_type,
                        base::OnceClosure callback) {
  const NativeWindow* native_window = window->window();
  if (!native_window)
    return;

  // (-1, -1) means showing on mouse location.
  gfx::Point location;
  if (x == -1 || y == -1) {
    location = display::Screen::Get()->GetCursorScreenPoint();
  } else {
    gfx::Point origin = native_window->GetContentBounds().origin();
    location = gfx::Point(origin.x() + x, origin.y() + y);
  }

  int flags = MenuRunner::CONTEXT_MENU | MenuRunner::HAS_MNEMONICS;

  // Make sure the Menu object would not be garbage-collected until the callback
  // has run.
  base::OnceClosure callback_with_ref = BindSelfToClosure(std::move(callback));

  // Show the menu.
  //
  // Note that while views::MenuRunner accepts RepeatingCallback as close
  // callback, it is fine passing OnceCallback to it because we reset the
  // menu runner immediately when the menu is closed.
  int32_t window_id = window->weak_map_id();
  auto close_callback = electron::AdaptCallbackForRepeating(
      base::BindOnce(&MenuViews::OnClosed, weak_factory_.GetWeakPtr(),
                     window_id, std::move(callback_with_ref)));
  auto& runner = menu_runners_[window_id] =
      std::make_unique<MenuRunner>(model(), flags, std::move(close_callback));
  runner->RunMenuAt(native_window->widget(), nullptr,
                    gfx::Rect{location, gfx::Size{}},
                    views::MenuAnchorPosition::kTopLeft, source_type);
}

void MenuViews::ClosePopupAt(int32_t window_id) {
  if (auto iter = menu_runners_.find(window_id); iter != menu_runners_.end()) {
    // Close the runner for the window.
    iter->second->Cancel();
    return;
  }

  if (window_id == -1) {
    // When -1 is passed in, close all opened runners.
    // Note: `Cancel()` invalidaes iters, so move() to a temp before looping
    auto tmp = std::move(menu_runners_);
    for (auto& [id, runner] : tmp)
      runner->Cancel();
  }
}

void MenuViews::OnClosed(int32_t window_id, base::OnceClosure callback) {
  menu_runners_.erase(window_id);
  std::move(callback).Run();
}

// static
Menu* Menu::New(gin::Arguments* args) {
  v8::Isolate* const isolate = args->isolate();
  Menu* menu = cppgc::MakeGarbageCollected<MenuViews>(
      isolate->GetCppHeap()->GetAllocationHandle(), args);
  gin_helper::CallMethod(isolate, menu, "_init");
  return menu;
}

}  // namespace electron::api
