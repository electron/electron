// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_VIEWS_WIN_CAPTION_BUTTON_H_
#define SHELL_BROWSER_UI_VIEWS_WIN_CAPTION_BUTTON_H_

#include "shell/browser/ui/views/title_bar.h"
#include "shell/browser/ui/views/view_ids.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/button/button.h"

namespace electron {

class WindowsCaptionButton : public views::Button {
 public:
  WindowsCaptionButton(TitleBar* title_bar,
                       ViewID button_type,
                       const base::string16& accessible_name);

  // views::Button:
  gfx::Size CalculatePreferredSize() const override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void PaintButtonContents(gfx::Canvas* canvas) override;

 private:
  // Returns the amount we should visually reserve on the left (right in RTL)
  // for spacing between buttons. We do this instead of repositioning the
  // buttons to avoid the sliver of deadspace that would result.
  int GetBetweenButtonSpacing() const;

  // Returns the order in which this button will be displayed (with 0 being
  // drawn farthest to the left, and larger indices being drawn to the right of
  // smaller indices).
  int GetButtonDisplayOrderIndex() const;

  // The base color to use for the button symbols and background blending. Uses
  // the more readable of black and white.
  SkColor GetBaseColor() const;

  // Paints the minimize/maximize/restore/close icon for the button.
  void PaintSymbol(gfx::Canvas* canvas);

  const views::Widget* widget() const { return title_bar_->GetWidget(); }

  TitleBar* title_bar_;
  ViewID button_type_;

  DISALLOW_COPY_AND_ASSIGN(WindowsCaptionButton);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_VIEWS_WIN_CAPTION_BUTTON_H_
