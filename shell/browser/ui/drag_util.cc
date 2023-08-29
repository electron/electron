// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/drag_util.h"

#include "ui/gfx/geometry/skia_conversions.h"

namespace electron {

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
