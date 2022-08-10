// Copyright (c) 2021 Ryan Gonzalez.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_CLIENT_FRAME_VIEW_LINUX_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_CLIENT_FRAME_VIEW_LINUX_H_

#include <array>
#include <memory>
#include <vector>

#include "base/scoped_observation.h"
#include "shell/browser/ui/views/frameless_view.h"
#include "ui/linux/linux_ui.h"
#include "ui/linux/nav_button_provider.h"
#include "ui/linux/window_button_order_observer.h"
#include "ui/linux/window_frame_provider.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_observer.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/frame_buttons.h"

namespace electron {

class ClientFrameViewLinux : public FramelessView,
                             public ui::NativeThemeObserver,
                             public ui::WindowButtonOrderObserver {
 public:
  static const char kViewClassName[];
  ClientFrameViewLinux();
  ~ClientFrameViewLinux() override;

  void Init(NativeWindowViews* window, views::Widget* frame) override;

  // These are here for ElectronDesktopWindowTreeHostLinux to use.
  gfx::Insets GetBorderDecorationInsets() const;
  gfx::Insets GetInputInsets() const;
  gfx::Rect GetWindowContentBounds() const;
  SkRRect GetRoundedWindowContentBounds() const;

 protected:
  // ui::NativeThemeObserver:
  void OnNativeThemeUpdated(ui::NativeTheme* observed_theme) override;

  // views::WindowButtonOrderObserver:
  void OnWindowButtonOrderingChange() override;

  // Overridden from FramelessView:
  int ResizingBorderHitTest(const gfx::Point& point) override;

  // Overridden from views::NonClientFrameView:
  gfx::Rect GetBoundsForClientView() const override;
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void GetWindowMask(const gfx::Size& size, SkPath* window_mask) override;
  void UpdateWindowTitle() override;
  void SizeConstraintsChanged() override;

  // Overridden from View:
  gfx::Size CalculatePreferredSize() const override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  void Layout() override;
  void OnPaint(gfx::Canvas* canvas) override;
  const char* GetClassName() const override;

  // Overriden from views::ViewTargeterDelegate
  views::View* TargetForRect(views::View* root, const gfx::Rect& rect) override;

 private:
  static constexpr int kNavButtonCount = 4;

  struct NavButton {
    ui::NavButtonProvider::FrameButtonDisplayType type;
    views::FrameButton frame_button;
    void (views::Widget::*callback)();
    int accessibility_id;
    int hit_test_id;
    views::ImageButton* button{nullptr};
  };

  struct ThemeValues {
    float window_border_radius;

    int titlebar_min_height;
    gfx::Insets titlebar_padding;

    SkColor title_color;
    gfx::Insets title_padding;

    int button_min_size;
    gfx::Insets button_padding;
  };

  void PaintAsActiveChanged();

  void UpdateThemeValues();

  enum class ButtonSide { kLeading, kTrailing };

  ui::NavButtonProvider::FrameButtonDisplayType GetButtonTypeToSkip() const;
  void UpdateButtonImages();
  void LayoutButtons();
  void LayoutButtonsOnSide(ButtonSide side,
                           gfx::Rect* remaining_content_bounds);

  gfx::Rect GetTitlebarBounds() const;
  gfx::Insets GetTitlebarContentInsets() const;
  gfx::Rect GetTitlebarContentBounds() const;

  gfx::Size SizeWithDecorations(gfx::Size size) const;

  ui::NativeTheme* theme_;
  ThemeValues theme_values_;

  views::Label* title_;

  std::unique_ptr<ui::NavButtonProvider> nav_button_provider_;
  std::array<NavButton, kNavButtonCount> nav_buttons_;

  std::vector<views::FrameButton> leading_frame_buttons_;
  std::vector<views::FrameButton> trailing_frame_buttons_;

  bool host_supports_client_frame_shadow_ = false;

  ui::WindowFrameProvider* frame_provider_;

  base::ScopedObservation<ui::NativeTheme, ui::NativeThemeObserver>
      native_theme_observer_{this};
  base::ScopedObservation<ui::LinuxUi,
                          ui::WindowButtonOrderObserver,
                          &ui::LinuxUi::AddWindowButtonOrderObserver,
                          &ui::LinuxUi::RemoveWindowButtonOrderObserver>
      window_button_order_observer_{this};

  base::CallbackListSubscription paint_as_active_changed_subscription_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_CLIENT_FRAME_VIEW_LINUX_H_
