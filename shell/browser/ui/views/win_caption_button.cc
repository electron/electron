// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/win_caption_button.h"

#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/scoped_canvas.h"

namespace electron {

namespace {

SkColor GetReadableFeatureColor(SkColor background_color) {
  return color_utils::GetLuma(background_color) < 128 ? SK_ColorWHITE
                                                      : SK_ColorBLACK;
}

}  // namespace

WindowsCaptionButton::WindowsCaptionButton(
    TitleBar* title_bar,
    ViewID button_type,
    const base::string16& accessible_name)
    : views::Button(title_bar),
      title_bar_(title_bar),
      button_type_(button_type) {
  SetID(button_type_);
  set_animate_on_state_change(true);
  SetAccessibleName(accessible_name);
}

gfx::Size WindowsCaptionButton::CalculatePreferredSize() const {
  int height = title_bar_->Height();
  int base_width = 0;
  switch (button_type_) {
    case VIEW_ID_HAMBURGER_BUTTON:
      base_width = height;
      break;
    case VIEW_ID_MINIMIZE_BUTTON:
    case VIEW_ID_MAXIMIZE_BUTTON:
    case VIEW_ID_RESTORE_BUTTON:
    case VIEW_ID_CLOSE_BUTTON:
      base_width = WindowFrameUtil::kWindows10GlassCaptionButtonWidth;
      break;
    default:
      NOTREACHED();
      return gfx::Size();
  }

  return gfx::Size(base_width + GetBetweenButtonSpacing(), height);
}

SkColor WindowsCaptionButton::GetBaseColor() const {
  // Get the theme's calculated custom control button background color
  // (as it takes into account images, etc).  If none is specified (likely when
  // there is no theme active), fall back to the titlebar color.
  const int control_button_bg_color_id =
      (widget()->ShouldPaintAsActive()
           ? ThemeProperties::COLOR_WINDOW_CONTROL_BUTTON_BACKGROUND_ACTIVE
           : ThemeProperties::COLOR_WINDOW_CONTROL_BUTTON_BACKGROUND_INACTIVE);
  const ui::ThemeProvider* theme_provider = GetThemeProvider();
  const bool has_custom_color =
      theme_provider->HasCustomColor(control_button_bg_color_id);
  const SkColor bg_color =
      (has_custom_color ? theme_provider->GetColor(control_button_bg_color_id)
                        : title_bar_->GetFrameColor());

  return GetReadableFeatureColor(bg_color);
}

void WindowsCaptionButton::OnPaintBackground(gfx::Canvas* canvas) {
  // Paint the background of the button (the semi-transparent rectangle that
  // appears when you hover or press the button).
  const ui::ThemeProvider* theme_provider = GetThemeProvider();
  const SkColor bg_color = theme_provider->GetColor(
      ThemeProperties::COLOR_CONTROL_BUTTON_BACKGROUND);
  const SkAlpha theme_alpha = SkColorGetA(bg_color);
  gfx::Rect bounds = GetContentsBounds();
  bounds.Inset(GetBetweenButtonSpacing(), 0, 0, 0);

  if (theme_alpha > 0) {
    canvas->FillRect(
        bounds,
        SkColorSetA(bg_color,
                    WindowFrameUtil::
                        CalculateWindows10GlassCaptionButtonBackgroundAlpha(
                            theme_alpha)));
  }

  SkColor base_color;
  SkAlpha hovered_alpha, pressed_alpha;
  if (button_type_ == VIEW_ID_CLOSE_BUTTON) {
    base_color = SkColorSetRGB(0xE8, 0x11, 0x23);
    hovered_alpha = SK_AlphaOPAQUE;
    pressed_alpha = 0x98;
  } else {
    // Match the native buttons.
    base_color = GetBaseColor();
    hovered_alpha = 0x1A;
    pressed_alpha = 0x33;

    if (theme_alpha > 0) {
      // Theme buttons have slightly increased opacity to make them stand out
      // against a visually-busy frame image.
      constexpr float kAlphaScale = 1.3f;
      hovered_alpha = gfx::ToRoundedInt(hovered_alpha * kAlphaScale);
      pressed_alpha = gfx::ToRoundedInt(pressed_alpha * kAlphaScale);
    }
  }

  SkAlpha alpha;
  if (state() == STATE_PRESSED)
    alpha = pressed_alpha;
  else
    alpha = gfx::Tween::IntValueBetween(hover_animation().GetCurrentValue(),
                                        SK_AlphaTRANSPARENT, hovered_alpha);
  canvas->FillRect(bounds, SkColorSetA(base_color, alpha));
}

void WindowsCaptionButton::PaintButtonContents(gfx::Canvas* canvas) {
  PaintSymbol(canvas);
}

int WindowsCaptionButton::GetBetweenButtonSpacing() const {
  const int display_order_index = GetButtonDisplayOrderIndex();
  return display_order_index == 0
             ? 0
             : WindowFrameUtil::kWindows10GlassCaptionButtonVisualSpacing;
}

int WindowsCaptionButton::GetButtonDisplayOrderIndex() const {
  int button_display_order = 0;
  switch (button_type_) {
    case VIEW_ID_HAMBURGER_BUTTON:
    case VIEW_ID_MINIMIZE_BUTTON:
      button_display_order = 0;
      break;
    case VIEW_ID_MAXIMIZE_BUTTON:
    case VIEW_ID_RESTORE_BUTTON:
      button_display_order = 1;
      break;
    case VIEW_ID_CLOSE_BUTTON:
      button_display_order = 2;
      break;
    default:
      NOTREACHED();
      return 0;
  }

  // Reverse the ordering if we're in RTL mode
  if (base::i18n::IsRTL())
    button_display_order = 2 - button_display_order;

  return button_display_order;
}

namespace {

// Canvas::DrawRect's stroke can bleed out of |rect|'s bounds, so this draws a
// rectangle inset such that the result is constrained to |rect|'s size.
void DrawRect(gfx::Canvas* canvas,
              const gfx::Rect& rect,
              const cc::PaintFlags& flags) {
  gfx::RectF rect_f(rect);
  float stroke_half_width = flags.getStrokeWidth() / 2;
  rect_f.Inset(stroke_half_width, stroke_half_width);
  canvas->DrawRect(rect_f, flags);
}

}  // namespace

void WindowsCaptionButton::PaintSymbol(gfx::Canvas* canvas) {
  SkColor symbol_color = GetBaseColor();
  if (!widget()->ShouldPaintAsActive() && state() != STATE_HOVERED &&
      state() != STATE_PRESSED) {
    symbol_color =
        SkColorSetA(symbol_color, TitleBar::kInactiveTitlebarFeatureAlpha);
  } else if (button_type_ == VIEW_ID_CLOSE_BUTTON &&
             hover_animation().is_animating()) {
    symbol_color = gfx::Tween::ColorValueBetween(
        hover_animation().GetCurrentValue(), symbol_color, SK_ColorWHITE);
  } else if (button_type_ == VIEW_ID_CLOSE_BUTTON &&
             (state() == STATE_HOVERED || state() == STATE_PRESSED)) {
    symbol_color = SK_ColorWHITE;
  }

  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();

  const int symbol_size_pixels = std::round(10 * scale);
  gfx::RectF bounds_rect(GetContentsBounds());
  bounds_rect.Scale(scale);
  gfx::Rect symbol_rect(gfx::ToEnclosingRect(bounds_rect));
  symbol_rect.ClampToCenteredSize(
      gfx::Size(symbol_size_pixels, symbol_size_pixels));

  cc::PaintFlags flags;
  flags.setAntiAlias(false);
  flags.setColor(symbol_color);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  // Stroke width jumps up a pixel every time we reach a new integral scale.
  const int stroke_width = std::floor(scale);
  flags.setStrokeWidth(stroke_width);

  switch (button_type_) {
    case VIEW_ID_HAMBURGER_BUTTON: {
      const int y = symbol_rect.CenterPoint().y();
      const int x1 = symbol_rect.x();
      const int x2 = symbol_rect.right();

      canvas->DrawLine(gfx::Point(x1, y - 5), gfx::Point(x2, y - 5), flags);
      canvas->DrawLine(gfx::Point(x1, y), gfx::Point(x2, y), flags);
      canvas->DrawLine(gfx::Point(x1, y + 5), gfx::Point(x2, y + 5), flags);

      return;
    }

    case VIEW_ID_MINIMIZE_BUTTON: {
      const int y = symbol_rect.CenterPoint().y();
      const gfx::Point p1 = gfx::Point(symbol_rect.x(), y);
      const gfx::Point p2 = gfx::Point(symbol_rect.right(), y);
      canvas->DrawLine(p1, p2, flags);
      return;
    }

    case VIEW_ID_MAXIMIZE_BUTTON:
      DrawRect(canvas, symbol_rect, flags);
      return;

    case VIEW_ID_RESTORE_BUTTON: {
      // Bottom left ("in front") square.
      const int separation = std::floor(2 * scale);
      symbol_rect.Inset(0, separation, separation, 0);
      DrawRect(canvas, symbol_rect, flags);

      // Top right ("behind") square.
      canvas->ClipRect(symbol_rect, SkClipOp::kDifference);
      symbol_rect.Offset(separation, -separation);
      DrawRect(canvas, symbol_rect, flags);
      return;
    }

    case VIEW_ID_CLOSE_BUTTON: {
      flags.setAntiAlias(true);
      // The close button's X is surrounded by a "halo" of transparent pixels.
      // When the X is white, the transparent pixels need to be a bit brighter
      // to be visible.
      const float stroke_halo =
          stroke_width * (symbol_color == SK_ColorWHITE ? 0.1f : 0.05f);
      flags.setStrokeWidth(stroke_width + stroke_halo);

      // TODO(bsep): This sometimes draws misaligned at fractional device scales
      // because the button's origin isn't necessarily aligned to pixels.
      canvas->ClipRect(symbol_rect);
      SkPath path;
      path.moveTo(symbol_rect.x(), symbol_rect.y());
      path.lineTo(symbol_rect.right(), symbol_rect.bottom());
      path.moveTo(symbol_rect.right(), symbol_rect.y());
      path.lineTo(symbol_rect.x(), symbol_rect.bottom());
      canvas->DrawPath(path, flags);
      return;
    }

    default:
      NOTREACHED();
      return;
  }
}

}  // namespace electron
