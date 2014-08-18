// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/submenu_button.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/text_utils.h"

namespace atom {

namespace {

// Filter out the "&" in menu label.
base::string16 FilterAccecelator(const base::string16& label) {
  base::string16 out;
  base::RemoveChars(label, base::ASCIIToUTF16("&").c_str(), &out);
  return out;
}

}  // namespace

SubmenuButton::SubmenuButton(views::ButtonListener* listener,
                             const base::string16& title,
                             views::MenuButtonListener* menu_button_listener)
    : views::MenuButton(listener, FilterAccecelator(title),
                        menu_button_listener, false),
      underline_start_(-1),
      underline_end_(-1),
      text_width_(0),
      text_height_(0),
      underline_color_(SK_ColorBLACK) {
  GetUnderlinePosition(title, &underline_start_, &underline_end_);
  gfx::Canvas::SizeStringInt(text(), font_list(), &text_width_,
                             &text_height_, 0, 0);
}

SubmenuButton::~SubmenuButton() {
}

void SubmenuButton::SetUnderlineColor(SkColor color) {
  underline_color_ = color;
}

void SubmenuButton::OnPaint(gfx::Canvas* canvas) {
  views::MenuButton::OnPaint(canvas);

  if (underline_start_ != underline_end_) {
    int padding = (width() - text_width_) / 2;
    int underline_height = (height() + text_height_) / 2 - 2;
    canvas->DrawLine(gfx::Point(underline_start_ + padding, underline_height),
                     gfx::Point(underline_end_ + padding, underline_height),
                     underline_color_);
  }
}

void SubmenuButton::GetUnderlinePosition(
    const base::string16& text, int* start, int* end) {
  int pos, span;
  gfx::RemoveAcceleratorChar(text, '&', &pos, &span);
  if (pos > -1 && span != 0) {
    GetCharacterPosition(text, pos, start);
    GetCharacterPosition(text, pos + span, end);
  }
}

void SubmenuButton::GetCharacterPosition(
    const base::string16& text, int index, int* pos) {
  int width, height;
  gfx::Canvas::SizeStringInt(text.substr(0, index), font_list(), &width,
                             &height, 0, 0);
  *pos = width;
}

}  // namespace atom
