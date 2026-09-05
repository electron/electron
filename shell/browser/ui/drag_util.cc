// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/drag_util.h"

#include <vector>

#include "third_party/blink/public/mojom/page/draggable_region.mojom.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/geometry/rect.h"

namespace electron {

// Convert draggable regions in raw format to SkRegion format.
//
// The regions arrive in document order and later ones win, so a `no-drag`
// rect punches a hole in the `drag` rects before it and a later `drag` rect
// can fill it again. Applying them one SkRegion::op() at a time is quadratic in
// the number of rects (each op is linear in the region built so far), which
// stalls the UI thread for ~100 ms at 10k rects. Instead, union each run of
// consecutive same-kind rects with SkRegion::setRects(), which is
// divide-and-conquer, and fold the runs in order; the result is identical.
SkRegion DraggableRegionsToSkRegion(
    const std::vector<blink::mojom::DraggableRegionPtr>& regions) {
  SkRegion sk_region;
  std::vector<SkIRect> run;
  for (size_t i = 0; i < regions.size();) {
    const bool draggable = regions[i]->draggable;
    run.clear();
    for (; i < regions.size() && regions[i]->draggable == draggable; ++i) {
      const gfx::Rect& bounds = regions[i]->bounds;
      run.push_back(SkIRect::MakeLTRB(bounds.x(), bounds.y(), bounds.right(),
                                      bounds.bottom()));
    }
    SkRegion run_region;
    run_region.setRects(run);
    sk_region.op(run_region,
                 draggable ? SkRegion::kUnion_Op : SkRegion::kDifference_Op);
  }
  return sk_region;
}

}  // namespace electron
