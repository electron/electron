// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/views/views_delegate.h"

#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"

#if defined(OS_LINUX)
#include "ui/views/linux_ui/linux_ui.h"
#endif

namespace brightray {

ViewsDelegate::ViewsDelegate() {
  DCHECK(!views::ViewsDelegate::views_delegate);
  views::ViewsDelegate::views_delegate = this;
}

ViewsDelegate::~ViewsDelegate() {
  DCHECK_EQ(views::ViewsDelegate::views_delegate, this);
  views::ViewsDelegate::views_delegate = NULL;
}

void ViewsDelegate::SaveWindowPlacement(const views::Widget* window,
                                        const std::string& window_name,
                                        const gfx::Rect& bounds,
                                        ui::WindowShowState show_state) {
}

bool ViewsDelegate::GetSavedWindowPlacement(
    const views::Widget* widget,
    const std::string& window_name,
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  return false;
}

void ViewsDelegate::NotifyAccessibilityEvent(
    views::View* view, ui::AXEvent event_type) {
}

void ViewsDelegate::NotifyMenuItemFocused(
    const base::string16& menu_name,
    const base::string16& menu_item_name,
    int item_index,
    int item_count,
    bool has_submenu) {
}

#if defined(OS_WIN)
HICON ViewsDelegate::GetDefaultWindowIcon() const {
  // Use current exe's icon as default window icon.
  return LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1  /* IDR_MAINFRAME */));
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

void ViewsDelegate::AddRef() {
}

void ViewsDelegate::ReleaseRef() {
}

content::WebContents* ViewsDelegate::CreateWebContents(
    content::BrowserContext* browser_context,
    content::SiteInstance* site_instance) {
  return NULL;
}

void ViewsDelegate::OnBeforeWidgetInit(
    views::Widget::InitParams* params,
    views::internal::NativeWidgetDelegate* delegate) {
  // If we already have a native_widget, we don't have to try to come
  // up with one.
  if (params->native_widget)
    return;

  // The native_widget is required when using aura.
  if (params->type == views::Widget::InitParams::TYPE_MENU ||
      (params->parent == NULL && params->context == NULL && !params->child))
    params->native_widget = new views::DesktopNativeWidgetAura(delegate);
}


base::TimeDelta ViewsDelegate::GetDefaultTextfieldObscuredRevealDuration() {
  return base::TimeDelta();
}

bool ViewsDelegate::WindowManagerProvidesTitleBar(bool maximized) {
#if defined(OS_LINUX)
  // On Ubuntu Unity, the system always provides a title bar for maximized
  // windows.
  views::LinuxUI* ui = views::LinuxUI::instance();
  return maximized && ui && ui->UnityIsRunning();
#else
  return false;
#endif
}

}  // namespace brightray
