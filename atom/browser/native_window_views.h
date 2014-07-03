// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_WINDOW_VIEWS_H_
#define ATOM_BROWSER_NATIVE_WINDOW_VIEWS_H_

#include "atom/browser/native_window.h"

#include <string>
#include <vector>

#include "ui/views/widget/widget_delegate.h"

namespace views {
class WebView;
class Widget;
}

namespace atom {

class NativeWindowViews : public NativeWindow,
                          public views::WidgetDelegateView {
 public:
  explicit NativeWindowViews(content::WebContents* web_contents,
                            const mate::Dictionary& options);
  virtual ~NativeWindowViews();

  // NativeWindow:
  virtual void Close() OVERRIDE;
  virtual void CloseImmediately() OVERRIDE;
  virtual void Move(const gfx::Rect& pos) OVERRIDE;
  virtual void Focus(bool focus) OVERRIDE;
  virtual bool IsFocused() OVERRIDE;
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;
  virtual bool IsVisible() OVERRIDE;
  virtual void Maximize() OVERRIDE;
  virtual void Unmaximize() OVERRIDE;
  virtual void Minimize() OVERRIDE;
  virtual void Restore() OVERRIDE;
  virtual void SetFullscreen(bool fullscreen) OVERRIDE;
  virtual bool IsFullscreen() OVERRIDE;
  virtual void SetSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Size GetSize() OVERRIDE;
  virtual void SetContentSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Size GetContentSize() OVERRIDE;
  virtual void SetMinimumSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Size GetMinimumSize() OVERRIDE;
  virtual void SetMaximumSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Size GetMaximumSize() OVERRIDE;
  virtual void SetResizable(bool resizable) OVERRIDE;
  virtual bool IsResizable() OVERRIDE;
  virtual void SetAlwaysOnTop(bool top) OVERRIDE;
  virtual bool IsAlwaysOnTop() OVERRIDE;
  virtual void Center() OVERRIDE;
  virtual void SetPosition(const gfx::Point& position) OVERRIDE;
  virtual gfx::Point GetPosition() OVERRIDE;
  virtual void SetTitle(const std::string& title) OVERRIDE;
  virtual std::string GetTitle() OVERRIDE;
  virtual void FlashFrame(bool flash) OVERRIDE;
  virtual void SetSkipTaskbar(bool skip) OVERRIDE;
  virtual void SetKiosk(bool kiosk) OVERRIDE;
  virtual bool IsKiosk() OVERRIDE;
  virtual gfx::NativeWindow GetNativeWindow() OVERRIDE;

 private:
  // NativeWindow:
  virtual void UpdateDraggableRegions(
      const std::vector<DraggableRegion>& regions) OVERRIDE;

  // views::View:
  virtual void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) OVERRIDE;

  // views::WidgetDelegate:
  virtual void DeleteDelegate() OVERRIDE;
  virtual views::View* GetInitiallyFocusedView() OVERRIDE;
  virtual bool CanResize() const OVERRIDE;
  virtual bool CanMaximize() const OVERRIDE;
  virtual base::string16 GetWindowTitle() const OVERRIDE;
  virtual bool ShouldHandleSystemCommands() const OVERRIDE;
  virtual gfx::ImageSkia GetWindowAppIcon() OVERRIDE;
  virtual gfx::ImageSkia GetWindowIcon() OVERRIDE;
  virtual views::Widget* GetWidget() OVERRIDE;
  virtual const views::Widget* GetWidget() const OVERRIDE;
  virtual views::View* GetContentsView() OVERRIDE;
  virtual views::ClientView* CreateClientView(views::Widget* widget) OVERRIDE;

  scoped_ptr<views::Widget> window_;
  views::WebView* web_view_;  // managed by window_.

  bool resizable_;
  std::string title_;
  gfx::Size minimum_size_;
  gfx::Size maximum_size_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowViews);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_WINDOW_VIEWS_H_
