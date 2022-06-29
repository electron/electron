// Copyright (c) 2022 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/win_icon_painter.h"

#include "base/numerics/safe_conversions.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/scoped_canvas.h"

namespace {

// Canvas::DrawRect's stroke can bleed out of |rect|'s bounds, so this draws a
// rectangle inset such that the result is constrained to |rect|'s size.
void DrawRect(gfx::Canvas* canvas,
              const gfx::Rect& rect,
              const cc::PaintFlags& flags) {
  gfx::RectF rect_f(rect);
  float stroke_half_width = flags.getStrokeWidth() / 2;
  rect_f.Inset(stroke_half_width);
  canvas->DrawRect(rect_f, flags);
}

void DrawRoundRect(gfx::Canvas* canvas,
                   const gfx::Rect& rect,
                   int radius,
                   const cc::PaintFlags& flags) {
  gfx::RectF rect_f(rect);
  float stroke_half_width = flags.getStrokeWidth() / 2;
  rect_f.Inset(stroke_half_width);
  canvas->DrawRoundRect(rect_f, radius, flags);
}

// Draws a path containing the top and right edges of a rounded rectangle.
void DrawRoundRectEdges(gfx::Canvas* canvas,
                        const gfx::Rect& rect,
                        float radius,
                        const cc::PaintFlags& flags) {
  gfx::RectF symbol_rect_f(rect);
  float stroke_half_width = flags.getStrokeWidth() / 2;
  symbol_rect_f.Inset(stroke_half_width);
  SkPath path;
  path.moveTo(symbol_rect_f.x(), symbol_rect_f.y());
  path.arcTo(symbol_rect_f.right(), symbol_rect_f.y(), symbol_rect_f.right(),
             symbol_rect_f.y() + radius, radius);
  path.lineTo(symbol_rect_f.right(), symbol_rect_f.bottom());
  canvas->DrawPath(path, flags);
}

}  // namespace

namespace electron {

WinIconPainter::WinIconPainter() = default;
WinIconPainter::~WinIconPainter() = default;

void WinIconPainter::PaintMinimizeIcon(gfx::Canvas* canvas,
                                       const gfx::Rect& symbol_rect,
                                       const cc::PaintFlags& flags) {
  const int y = symbol_rect.CenterPoint().y();
  const gfx::Point p1 = gfx::Point(symbol_rect.x(), y);
  const gfx::Point p2 = gfx::Point(symbol_rect.right(), y);
  canvas->DrawLine(p1, p2, flags);
}

void WinIconPainter::PaintMaximizeIcon(gfx::Canvas* canvas,
                                       const gfx::Rect& symbol_rect,
                                       const cc::PaintFlags& flags) {
  DrawRect(canvas, symbol_rect, flags);
}

void WinIconPainter::PaintRestoreIcon(gfx::Canvas* canvas,
                                      const gfx::Rect& symbol_rect,
                                      const cc::PaintFlags& flags) {
  const int separation = std::floor(2 * canvas->image_scale());
  symbol_rect.Inset(gfx::Insets::TLBR(separation, 0, 0, separation));

  // Bottom left ("in front") square.
  DrawRect(canvas, symbol_rect, flags);

  // Top right ("behind") square.
  canvas->ClipRect(symbol_rect, SkClipOp::kDifference);
  symbol_rect.Offset(separation, -separation);
  DrawRect(canvas, symbol_rect, flags);
}

void WinIconPainter::PaintCloseIcon(gfx::Canvas* canvas,
                                    const gfx::Rect& symbol_rect,
                                    const cc::PaintFlags& flags) {
  // TODO(bsep): This sometimes draws misaligned at fractional device scales
  // because the button's origin isn't necessarily aligned to pixels.
  flags.setAntiAlias(true);
  canvas->ClipRect(symbol_rect);
  SkPath path;
  path.moveTo(symbol_rect.x(), symbol_rect.y());
  path.lineTo(symbol_rect.right(), symbol_rect.bottom());
  path.moveTo(symbol_rect.right(), symbol_rect.y());
  path.lineTo(symbol_rect.x(), symbol_rect.bottom());
  canvas->DrawPath(path, flags);
}

Win11IconPainter::Win11IconPainter() = default;
Win11IconPainter::~Win11IconPainter() = default;

void Win11IconPainter::PaintMaximizeIcon(gfx::Canvas* canvas,
                                         const gfx::Rect& symbol_rect,
                                         const cc::PaintFlags& flags) {
  flags.setAntiAlias(true);
  const float corner_radius = 2 * canvas->image_scale();
  DrawRoundRect(canvas, symbol_rect, corner_radius, flags);
}

void Win11IconPainter::PaintRestoreIcon(gfx::Canvas* canvas,
                                        const gfx::Rect& symbol_rect,
                                        const cc::PaintFlags& flags) {
  const int separation = std::floor(2 * canvas->image_scale());
  symbol_rect.Inset(gfx::Insets::TLBR(separation, 0, 0, separation));

  flags.setAntiAlias(true);
  // Bottom left ("in front") rounded square.
  const float bottom_rect_radius = 1 * canvas->image_scale();
  DrawRoundRect(canvas, symbol_rect, bottom_rect_radius, flags);

  // Top right ("behind") top+right edges of rounded square (2.5x).
  symbol_rect.Offset(separation, -separation);
  // Apply inset to left+bottom edges since we don't draw arcs for those edges
  constexpr int top_rect_inset = 1;
  symbol_rect.Inset(gfx::Insets::TLBR(0, top_rect_inset, top_rect_inset, 0));

  const float top_rect_radius = 2.5f * canvas->image_scale();
  DrawRoundRectEdges(canvas, symbol_rect, top_rect_radius, flags);
}
}  // namespace electron
