// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/drag_util.h"

#include <memory>

#include "ui/gfx/geometry/skia_conversions.h"

namespace electron {

// Return a vector of non-draggable regions that fill a window of size
// |width| by |height|, but leave gaps where the window should be draggable.
std::vector<gfx::Rect> CalculateNonDraggableRegions(
    std::unique_ptr<SkRegion> draggable,
    int width,
    int height) {
  std::vector<gfx::Rect> result;
  SkRegion non_draggable;
  non_draggable.op({0, 0, width, height}, SkRegion::kUnion_Op);
  non_draggable.op(*draggable, SkRegion::kDifference_Op);
  for (SkRegion::Iterator it(non_draggable); !it.done(); it.next()) {
    result.push_back(gfx::SkIRectToRect(it.rect()));
  }
  return result;
}

// Convert draggable regions in raw format to SkRegion format.
std::unique_ptr<SkRegion> DraggableRegionsToSkRegion(
    const std::vector<mojom::DraggableRegionPtr>& regions) {
  auto sk_region = std::make_unique<SkRegion>();
  for (const auto& region : regions) {
    sk_region->op(
        SkIRect::MakeLTRB(region->bounds.x(), region->bounds.y(),
                          region->bounds.right(), region->bounds.bottom()),
        region->draggable ? SkRegion::kUnion_Op : SkRegion::kDifference_Op);
  }
  return sk_region;
}

}  // namespace electron
