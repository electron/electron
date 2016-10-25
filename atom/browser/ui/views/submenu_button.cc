// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/submenu_button.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/controls/button/label_button_border.h"

namespace atom {

namespace {

// Filter out the "&" in menu label.
base::string16 FilterAccelerator(const base::string16& label) {
  base::string16 out;
  base::RemoveChars(label, base::ASCIIToUTF16("&").c_str(), &out);
  return out;
}

}  // namespace

SubmenuButton::SubmenuButton(const base::string16& title,
                             views::MenuButtonListener* menu_button_listener,
                             const SkColor& background_color)
    : views::MenuButton(FilterAccelerator(title),
                        menu_button_listener, false),
      accelerator_(0),
      show_underline_(false),
      underline_start_(-1),
      underline_end_(-1),
      text_width_(0),
      text_height_(0),
      underline_color_(SK_ColorBLACK),
      background_color_(background_color) {
#if defined(OS_LINUX)
  // Dont' use native style border.
  SetBorder(std::move(CreateDefaultBorder()));
#endif

  if (GetUnderlinePosition(title, &accelerator_, &underline_start_,
                           &underline_end_))
    gfx::Canvas::SizeStringInt(GetText(), GetFontList(), &text_width_,
                               &text_height_, 0, 0);

  SetHasInkDrop(true);
  set_ink_drop_base_color(
      color_utils::BlendTowardOppositeLuma(background_color_, 0x61));
}

SubmenuButton::~SubmenuButton() {
}

std::unique_ptr<views::InkDropRipple> SubmenuButton::CreateInkDropRipple()
    const {
  return base::MakeUnique<views::FloodFillInkDropRipple>(
      GetLocalBounds(),
      GetInkDropCenterBasedOnLastEvent(),
      GetInkDropBaseColor(),
      ink_drop_visible_opacity());
}

std::unique_ptr<views::InkDropHighlight>
    SubmenuButton::CreateInkDropHighlight() const {
  if (!ShouldShowInkDropHighlight())
    return nullptr;

  gfx::Size size = GetLocalBounds().size();
  return base::MakeUnique<views::InkDropHighlight>(
      size,
      kInkDropSmallCornerRadius,
      gfx::RectF(gfx::SizeF(size)).CenterPoint(),
      GetInkDropBaseColor());
}

bool SubmenuButton::ShouldShowInkDropForFocus() const {
  return false;
}

void SubmenuButton::SetAcceleratorVisibility(bool visible) {
  if (visible == show_underline_)
    return;

  show_underline_ = visible;
  SchedulePaint();
}

void SubmenuButton::SetUnderlineColor(SkColor color) {
  underline_color_ = color;
}

void SubmenuButton::OnPaint(gfx::Canvas* canvas) {
  views::MenuButton::OnPaint(canvas);

  if (show_underline_ && (underline_start_ != underline_end_)) {
    int padding = (width() - text_width_) / 2;
    int underline_height = (height() + text_height_) / 2 - 2;
    canvas->DrawLine(gfx::Point(underline_start_ + padding, underline_height),
                     gfx::Point(underline_end_ + padding, underline_height),
                     underline_color_);
  }
}

bool SubmenuButton::GetUnderlinePosition(const base::string16& text,
                                         base::char16* accelerator,
                                         int* start, int* end) {
  int pos, span;
  base::string16 trimmed = gfx::RemoveAcceleratorChar(text, '&', &pos, &span);
  if (pos > -1 && span != 0) {
    *accelerator = base::ToUpperASCII(trimmed[pos]);
    GetCharacterPosition(trimmed, pos, start);
    GetCharacterPosition(trimmed, pos + span, end);
    return true;
  }

  return false;
}

void SubmenuButton::GetCharacterPosition(
    const base::string16& text, int index, int* pos) {
  int height;
  gfx::Canvas::SizeStringInt(text.substr(0, index), GetFontList(), pos, &height,
                             0, 0);
}

}  // namespace atom
