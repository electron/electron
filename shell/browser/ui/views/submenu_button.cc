// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/submenu_button.h"

#include <memory>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/controls/button/label_button_border.h"

namespace atom {

SubmenuButton::SubmenuButton(const base::string16& title,
                             views::MenuButtonListener* menu_button_listener,
                             const SkColor& background_color)
    : views::MenuButton(gfx::RemoveAcceleratorChar(title, '&', NULL, NULL),
                        menu_button_listener),
      background_color_(background_color) {
#if defined(OS_LINUX)
  // Dont' use native style border.
  SetBorder(CreateDefaultBorder());
#endif

  if (GetUnderlinePosition(title, &accelerator_, &underline_start_,
                           &underline_end_))
    gfx::Canvas::SizeStringInt(GetText(), gfx::FontList(), &text_width_,
                               &text_height_, 0, 0);

  SetInkDropMode(InkDropMode::ON);
  set_ink_drop_base_color(
      color_utils::BlendTowardMaxContrast(background_color_, 0x81));
}

SubmenuButton::~SubmenuButton() {}

std::unique_ptr<views::InkDropRipple> SubmenuButton::CreateInkDropRipple()
    const {
  std::unique_ptr<views::InkDropRipple> ripple(
      new views::FloodFillInkDropRipple(
          size(), GetInkDropCenterBasedOnLastEvent(), GetInkDropBaseColor(),
          ink_drop_visible_opacity()));
  return ripple;
}

std::unique_ptr<views::InkDrop> SubmenuButton::CreateInkDrop() {
  std::unique_ptr<views::InkDropImpl> ink_drop =
      views::Button::CreateDefaultInkDropImpl();
  ink_drop->SetShowHighlightOnHover(false);
  ink_drop->SetShowHighlightOnFocus(true);
  return std::move(ink_drop);
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

void SubmenuButton::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->SetName(GetAccessibleName());
  node_data->role = ax::mojom::Role::kPopUpButton;
}

void SubmenuButton::PaintButtonContents(gfx::Canvas* canvas) {
  views::MenuButton::PaintButtonContents(canvas);

  if (show_underline_ && (underline_start_ != underline_end_)) {
    float padding = (width() - text_width_) / 2;
    float underline_height = (height() + text_height_) / 2 - 2;
    canvas->DrawSharpLine(
        gfx::PointF(underline_start_ + padding, underline_height),
        gfx::PointF(underline_end_ + padding, underline_height),
        underline_color_);
  }
}

bool SubmenuButton::GetUnderlinePosition(const base::string16& text,
                                         base::char16* accelerator,
                                         int* start,
                                         int* end) const {
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

void SubmenuButton::GetCharacterPosition(const base::string16& text,
                                         int index,
                                         int* pos) const {
  int height = 0;
  gfx::Canvas::SizeStringInt(text.substr(0, index), gfx::FontList(), pos,
                             &height, 0, 0);
}

}  // namespace atom
