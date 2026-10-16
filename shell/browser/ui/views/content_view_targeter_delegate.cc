// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/content_view_targeter_delegate.h"

#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/views/view.h"

namespace electron {

bool ContentViewTargeterDelegate::DoesIntersectRect(
    const views::View* target,
    const gfx::Rect& rect) const {
  if (!views::ViewTargeterDelegate::DoesIntersectRect(target, rect)) {
    return false;
  }
  for (const views::View* child : target->children()) {
    if (!child->GetVisible() || !child->GetCanProcessEventsWithinSubtree()) {
      continue;
    }
    gfx::RectF rect_in_child(rect);
    views::View::ConvertRectToTarget(target, child, &rect_in_child);
    if (child->HitTestRect(gfx::ToEnclosingRect(rect_in_child))) {
      return true;
    }
  }
  return false;
}

}  // namespace electron
