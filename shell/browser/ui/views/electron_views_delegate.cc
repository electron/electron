// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/ui/views/electron_views_delegate.h"

#include <memory>

#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/widget/native_widget_aura.h"
#include "ui/views/window/default_frame_view.h"

namespace electron {

ViewsDelegate::ViewsDelegate() = default;

ViewsDelegate::~ViewsDelegate() = default;

void ViewsDelegate::SaveWindowPlacement(const views::Widget* window,
                                        const std::string& window_name,
                                        const gfx::Rect& bounds,
                                        ui::mojom::WindowShowState show_state) {
}

bool ViewsDelegate::GetSavedWindowPlacement(
    const views::Widget* widget,
    const std::string& window_name,
    gfx::Rect* bounds,
    ui::mojom::WindowShowState* show_state) const {
  return false;
}

void ViewsDelegate::NotifyMenuItemFocused(const std::u16string& menu_name,
                                          const std::u16string& menu_item_name,
                                          int item_index,
                                          int item_count,
                                          bool has_submenu) {}

#if BUILDFLAG(IS_LINUX) && !BUILDFLAG(IS_CHROMEOS)
gfx::ImageSkia* ViewsDelegate::GetDefaultWindowIcon() const {
  return nullptr;
}
#endif

std::unique_ptr<views::FrameView> ViewsDelegate::CreateDefaultFrameView(
    views::Widget* widget) {
  return std::make_unique<views::DefaultFrameView>(widget);
}

void ViewsDelegate::OnBeforeWidgetInit(
    views::Widget::InitParams* params,
    views::internal::NativeWidgetDelegate* delegate) {
  // If we already have a native_widget, we don't have to try to come
  // up with one.
  if (params->native_widget)
    return;

  if (params->parent && params->type != views::Widget::InitParams::TYPE_MENU &&
      params->type != views::Widget::InitParams::TYPE_TOOLTIP) {
    params->native_widget = new views::NativeWidgetAura(delegate);
  } else {
    params->native_widget = new views::DesktopNativeWidgetAura(delegate);
  }
}

}  // namespace electron
