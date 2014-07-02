// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_menu_gtk.h"

#include "atom/browser/native_window_gtk.h"
#include "content/public/browser/render_widget_host_view.h"
#include "ui/gfx/point.h"
#include "ui/gfx/screen.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

MenuGtk::MenuGtk() {
}

void MenuGtk::Popup(Window* window) {
  /*
  uint32_t triggering_event_time;
  gfx::Point point;

  NativeWindow* native_window = window->window();
  GdkEventButton* event = native_window->GetWebContents()->
      GetRenderWidgetHostView()->GetLastMouseDown();
  if (event) {
    triggering_event_time = event->time;
    point = gfx::Point(event->x_root, event->y_root);
  } else {
    triggering_event_time = GDK_CURRENT_TIME;
    point = gfx::Screen::GetNativeScreen()->GetCursorScreenPoint();
  }

  menu_gtk_.reset(new ::MenuGtk(this, model_.get()));
  menu_gtk_->PopupAsContext(point, triggering_event_time);
  */
}

void MenuGtk::AttachToWindow(Window* window) {
  // static_cast<NativeWindowGtk*>(window->window())->SetMenu(model_.get());
}

// static
mate::Wrappable* Menu::Create() {
  return new MenuGtk();
}

}  // namespace api

}  // namespace atom
