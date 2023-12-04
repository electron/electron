// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/submenu_button.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_host.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/controls/button/label_button_border.h"

namespace electron {

SubmenuButton::SubmenuButton(PressedCallback callback,
                             const std::u16string& title,
                             const SkColor& background_color)
    : views::MenuButton(callback, gfx::RemoveAccelerator(title)),
      background_color_(background_color) {
#if BUILDFLAG(IS_LINUX)
  // Dont' use native style border.
  SetBorder(CreateDefaultBorder());
#endif

  if (GetUnderlinePosition(title, &accelerator_, &underline_start_,
                           &underline_end_))
    gfx::Canvas::SizeStringInt(GetText(), gfx::FontList(), &text_width_,
                               &text_height_, 0, 0);

  views::InkDropHost* ink_drop = views::InkDrop::Get(this);
  ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
  ink_drop->SetBaseColor(
      color_utils::BlendTowardMaxContrast(background_color_, 0x81));
  views::InkDrop::UseInkDropForFloodFillRipple(ink_drop, false, true);
}

SubmenuButton::~SubmenuButton() = default;

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
  node_data->role = ax::mojom::Role::kPopUpButton;
  node_data->SetName(GetAccessibleName());
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

bool SubmenuButton::GetUnderlinePosition(const std::u16string& text,
                                         char16_t* accelerator,
                                         int* start,
                                         int* end) const {
  int pos, span;
  std::u16string trimmed =
      gfx::LocateAndRemoveAcceleratorChar(text, &pos, &span);
  if (pos > -1 && span != 0) {
    *accelerator = base::ToUpperASCII(trimmed[pos]);
    GetCharacterPosition(trimmed, pos, start);
    GetCharacterPosition(trimmed, pos + span, end);
    return true;
  }

  return false;
}

void SubmenuButton::GetCharacterPosition(const std::u16string& text,
                                         int index,
                                         int* pos) const {
  int height = 0;
  gfx::Canvas::SizeStringInt(text.substr(0, index), gfx::FontList(), pos,
                             &height, 0, 0);
}

BEGIN_METADATA(SubmenuButton)
END_METADATA

}  // namespace electron
