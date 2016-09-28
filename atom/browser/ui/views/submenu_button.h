// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_VIEWS_SUBMENU_BUTTON_H_
#define ATOM_BROWSER_UI_VIEWS_SUBMENU_BUTTON_H_

#include "ui/views/controls/button/menu_button.h"
#include "ui/views/animation/ink_drop_highlight.h"

namespace atom {

// Special button that used by menu bar to show submenus.
class SubmenuButton : public views::MenuButton {
 public:
  SubmenuButton(views::ButtonListener* listener,
                const base::string16& title,
                views::MenuButtonListener* menu_button_listener,
                const SkColor& background_color);
  virtual ~SubmenuButton();

  void SetAcceleratorVisibility(bool visible);
  void SetUnderlineColor(SkColor color);

  void SetEnabledColor(SkColor color);
  void SetBackgroundColor(SkColor color);

  base::char16 accelerator() const { return accelerator_; }

  // views::MenuButton:
  void OnPaint(gfx::Canvas* canvas) override;

  // views::InkDropHostView:
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  std::unique_ptr<views::InkDropHighlight> CreateInkDropHighlight()
     const override;
  bool ShouldShowInkDropForFocus() const override;

 private:
  bool GetUnderlinePosition(const base::string16& text,
                            base::char16* accelerator,
                            int* start, int* end);
  void GetCharacterPosition(
      const base::string16& text, int index, int* pos);

  base::char16 accelerator_;

  bool show_underline_;

  int underline_start_;
  int underline_end_;
  int text_width_;
  int text_height_;
  SkColor underline_color_;
  SkColor background_color_;

  DISALLOW_COPY_AND_ASSIGN(SubmenuButton);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_VIEWS_SUBMENU_BUTTON_H_
