// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_VIEWS_TITLE_BAR_H_
#define SHELL_BROWSER_UI_VIEWS_TITLE_BAR_H_

#include "shell/browser/native_window.h"
#include "shell/browser/native_window_observer.h"
#include "shell/browser/ui/views/root_view.h"
#include "shell/browser/ui/views/view_ids.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/accessible_pane_view.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class Button;
class MenuButton;
}  // namespace views

namespace electron {

class NativeWindow;
class WindowsCaptionButton;

class IconView : public views::Button {
 public:
  static const char kViewClassName[];

  explicit IconView(TitleBar* title_bar, NativeWindow* window);

 private:
  // views::Button:
  gfx::Size CalculatePreferredSize() const override;
  const char* GetClassName() const override;
  void PaintButtonContents(gfx::Canvas* canvas) override;

  void PaintIcon(gfx::Canvas* canvas, const gfx::ImageSkia& image);

  const views::Widget* widget() const;

  TitleBar* title_bar_;
  NativeWindow* window_;

  DISALLOW_COPY_AND_ASSIGN(IconView);
};

class TitleBar : public views::View,
                 public views::ButtonListener,
                 public NativeWindowObserver {
 public:
  // Alpha to use for features in the titlebar (the window title and caption
  // buttons) when the window is inactive. They are opaque when active.
  static constexpr SkAlpha kInactiveTitlebarFeatureAlpha = 0x66;

  static const char kViewClassName[];

  TitleBar(RootView* root_view);
  ~TitleBar() override;

  int Height() const;
  SkColor GetFrameColor() const;
  int NonClientHitTest(const gfx::Point& point);
  void UpdateWindowIcon();
  void UpdateWindowTitle();

  void RequestSystemMenuAt(const gfx::Point& point);
  void RequestSystemMenu();

 protected:
  // views::View:
  const char* GetClassName() const override;
  void AddedToWidget() override;
  void Layout() override;

  // NativeWindowObserver
  void OnPaintAsActiveChanged(bool paint_as_active) override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  WindowsCaptionButton* CreateCaptionButton(ViewID button_type,
                                            int accessible_name_resource_id);

  void LayoutTitleBar();
  void LayoutCaptionButtons();
  void LayoutCaptionButton(WindowsCaptionButton* button, int previous_button_x);

  bool ShowTitle() const;
  bool ShowIcon() const;
  bool ShowHamburgerMenu() const;

 private:
  void UpdateViewColors();

  RootView* root_view_;

  WindowsCaptionButton* hamburger_button_;

  IconView* window_icon_;
  views::Label* window_title_;
  gfx::HorizontalAlignment title_alignment_;

  WindowsCaptionButton* minimize_button_;
  WindowsCaptionButton* maximize_button_;
  WindowsCaptionButton* restore_button_;
  WindowsCaptionButton* close_button_;

  DISALLOW_COPY_AND_ASSIGN(TitleBar);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_VIEWS_TITLE_BAR_H_
