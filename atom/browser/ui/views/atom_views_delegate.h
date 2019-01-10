// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ATOM_BROWSER_UI_VIEWS_ATOM_VIEWS_DELEGATE_H_
#define ATOM_BROWSER_UI_VIEWS_ATOM_VIEWS_DELEGATE_H_

#include <string>

#include "base/compiler_specific.h"
#include "ui/views/views_delegate.h"

namespace atom {

class ViewsDelegate : public views::ViewsDelegate {
 public:
  ViewsDelegate();
  ~ViewsDelegate() override;

 protected:
  // views::ViewsDelegate:
  void SaveWindowPlacement(const views::Widget* window,
                           const std::string& window_name,
                           const gfx::Rect& bounds,
                           ui::WindowShowState show_state) override;
  bool GetSavedWindowPlacement(const views::Widget* widget,
                               const std::string& window_name,
                               gfx::Rect* bounds,
                               ui::WindowShowState* show_state) const override;
  void NotifyMenuItemFocused(const base::string16& menu_name,
                             const base::string16& menu_item_name,
                             int item_index,
                             int item_count,
                             bool has_submenu) override;

#if defined(OS_WIN)
  HICON GetDefaultWindowIcon() const override;
  HICON GetSmallWindowIcon() const override;
  bool IsWindowInMetro(gfx::NativeWindow window) const override;
#elif defined(OS_LINUX) && !defined(OS_CHROMEOS)
  gfx::ImageSkia* GetDefaultWindowIcon() const override;
#endif
  views::NonClientFrameView* CreateDefaultNonClientFrameView(
      views::Widget* widget) override;
  void AddRef() override;
  void ReleaseRef() override;
  void OnBeforeWidgetInit(
      views::Widget::InitParams* params,
      views::internal::NativeWidgetDelegate* delegate) override;
  bool WindowManagerProvidesTitleBar(bool maximized) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ViewsDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_ATOM_VIEWS_DELEGATE_H_
