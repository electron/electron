// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_VIEWS_VIEWS_DELEGATE_H_
#define BRIGHTRAY_BROWSER_VIEWS_VIEWS_DELEGATE_H_

#include "base/compiler_specific.h"
#include "ui/views/views_delegate.h"

namespace brightray {

class ViewsDelegate : public views::ViewsDelegate {
 public:
  ViewsDelegate();
  virtual ~ViewsDelegate();

 protected:
  // views::ViewsDelegate:
  virtual void SaveWindowPlacement(const views::Widget* window,
                                   const std::string& window_name,
                                   const gfx::Rect& bounds,
                                   ui::WindowShowState show_state) OVERRIDE;
  virtual bool GetSavedWindowPlacement(
      const views::Widget* widget,
      const std::string& window_name,
      gfx::Rect* bounds,
      ui::WindowShowState* show_state) const OVERRIDE;
  virtual void NotifyAccessibilityEvent(
      views::View* view, ui::AXEvent event_type) OVERRIDE;
  virtual void NotifyMenuItemFocused(const base::string16& menu_name,
                                     const base::string16& menu_item_name,
                                     int item_index,
                                     int item_count,
                                     bool has_submenu) OVERRIDE;

#if defined(OS_WIN)
  virtual HICON GetDefaultWindowIcon() const OVERRIDE;
  virtual bool IsWindowInMetro(gfx::NativeWindow window) const OVERRIDE;
#elif defined(OS_LINUX) && !defined(OS_CHROMEOS)
  virtual gfx::ImageSkia* GetDefaultWindowIcon() const OVERRIDE;
#endif
  virtual views::NonClientFrameView* CreateDefaultNonClientFrameView(
      views::Widget* widget) OVERRIDE;
  virtual void AddRef() OVERRIDE;
  virtual void ReleaseRef() OVERRIDE;
  virtual content::WebContents* CreateWebContents(
      content::BrowserContext* browser_context,
      content::SiteInstance* site_instance) OVERRIDE;
  virtual void OnBeforeWidgetInit(
      views::Widget::InitParams* params,
      views::internal::NativeWidgetDelegate* delegate) OVERRIDE;
  virtual base::TimeDelta GetDefaultTextfieldObscuredRevealDuration() OVERRIDE;
  virtual bool WindowManagerProvidesTitleBar(bool maximized) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(ViewsDelegate);
};



}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_VIEWS_VIEWS_DELEGATE_H_
