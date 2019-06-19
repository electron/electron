// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "atom/browser/ui/views/atom_views_delegate.h"

#include <memory>

#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/widget/native_widget_aura.h"

#if defined(OS_LINUX)
#include "base/environment.h"
#include "base/nix/xdg_util.h"
#include "ui/views/linux_ui/linux_ui.h"
#endif

namespace {

#if defined(OS_LINUX)
bool IsDesktopEnvironmentUnity() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  base::nix::DesktopEnvironment desktop_env =
      base::nix::GetDesktopEnvironment(env.get());
  return desktop_env == base::nix::DESKTOP_ENVIRONMENT_UNITY;
}
#endif

}  // namespace

namespace atom {

ViewsDelegate::ViewsDelegate() {}

ViewsDelegate::~ViewsDelegate() {}

void ViewsDelegate::SaveWindowPlacement(const views::Widget* window,
                                        const std::string& window_name,
                                        const gfx::Rect& bounds,
                                        ui::WindowShowState show_state) {}

bool ViewsDelegate::GetSavedWindowPlacement(
    const views::Widget* widget,
    const std::string& window_name,
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  return false;
}

void ViewsDelegate::NotifyMenuItemFocused(const base::string16& menu_name,
                                          const base::string16& menu_item_name,
                                          int item_index,
                                          int item_count,
                                          bool has_submenu) {}

#if defined(OS_WIN)
HICON ViewsDelegate::GetDefaultWindowIcon() const {
  // Use current exe's icon as default window icon.
  return LoadIcon(GetModuleHandle(NULL),
                  MAKEINTRESOURCE(1 /* IDR_MAINFRAME */));
}

HICON ViewsDelegate::GetSmallWindowIcon() const {
  return GetDefaultWindowIcon();
}

bool ViewsDelegate::IsWindowInMetro(gfx::NativeWindow window) const {
  return false;
}

#elif defined(OS_LINUX) && !defined(OS_CHROMEOS)
gfx::ImageSkia* ViewsDelegate::GetDefaultWindowIcon() const {
  return NULL;
}
#endif

views::NonClientFrameView* ViewsDelegate::CreateDefaultNonClientFrameView(
    views::Widget* widget) {
  return NULL;
}

void ViewsDelegate::AddRef() {}

void ViewsDelegate::ReleaseRef() {}

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

bool ViewsDelegate::WindowManagerProvidesTitleBar(bool maximized) {
#if defined(OS_LINUX)
  // On Ubuntu Unity, the system always provides a title bar for maximized
  // windows.
  if (!maximized)
    return false;
  static bool is_desktop_environment_unity = IsDesktopEnvironmentUnity();
  return is_desktop_environment_unity;
#else
  return false;
#endif
}

}  // namespace atom
