// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/drag_util.h"

#include <vector>

#include "base/containers/span.h"
#include "third_party/blink/public/mojom/page/draggable_region.mojom.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/geometry/rect.h"

namespace electron {

namespace {

// TODO(MarshallOfSound): this is a copy of content::DraggableRegionsToSkRegion
// from https://chromium-review.googlesource.com/c/chromium/src/+/8360195.
// Once that lands and rolls into Electron, delete everything in this
// namespace and make DraggableRegionsToSkRegion() below a call to
// content::DraggableRegionsToSkRegion(regions).

SkIRect ToSkIRect(const gfx::Rect& rect) {
  return SkIRect::MakeLTRB(rect.x(), rect.y(), rect.right(), rect.bottom());
}

// The renderer sends an ordered list of rects. A draggable rect adds its area
// to the region and a non-draggable rect removes its area. Where rects
// overlap, the later rect wins, so a point ends up draggable exactly when the
// last rect in the list that contains it is draggable.
//
// A SequenceRegion describes one contiguous part of the list. `result` is the
// region that part produces when applied in order to an empty region.
// `covered` is the union of all its rects, draggable or not; `result` always
// lies inside `covered`. If every rect in the part is draggable the two are
// equal, so only `result` is stored and Covered() returns it.
//
// Applying the first half and then the second half of a list: a point outside
// every rect of the second half is draggable exactly when it is in
// `first.result`; a point inside a rect of the second half is draggable
// exactly when it is in `second.result`. So the whole is `first.result`, minus
// `second.covered`, plus `second.result`, and each half can be built on its
// own. That is O(n log n) region operations however draggable and
// non-draggable rects interleave, where one SkRegion::op() per rect is O(n^2)
// because each op walks the whole region built so far.
struct SequenceRegion {
  const SkRegion& Covered() const { return all_draggable ? result : covered; }

  SkRegion result;
  SkRegion covered;
  bool all_draggable = false;
};

SequenceRegion Build(
    base::span<const blink::mojom::DraggableRegionPtr> regions) {
  SequenceRegion out;
  if (regions.size() == 1) {
    const SkIRect rect = ToSkIRect(regions[0]->bounds);
    if (regions[0]->draggable) {
      out.result.setRect(rect);
      out.all_draggable = true;
    } else {
      out.covered.setRect(rect);
    }
    return out;
  }

  const size_t mid = regions.size() / 2;
  const SequenceRegion first = Build(regions.first(mid));
  const SequenceRegion second = Build(regions.subspan(mid));
  if (first.all_draggable && second.all_draggable) {
    out.result.op(first.result, second.result, SkRegion::kUnion_Op);
    out.all_draggable = true;
    return out;
  }
  if (second.result.isEmpty()) {
    out.result.op(first.result, second.Covered(), SkRegion::kDifference_Op);
  } else if (second.all_draggable) {
    out.result.op(first.result, second.result, SkRegion::kUnion_Op);
  } else {
    SkRegion first_outside_second;
    first_outside_second.op(first.result, second.covered,
                            SkRegion::kDifference_Op);
    out.result.op(first_outside_second, second.result, SkRegion::kUnion_Op);
  }
  out.covered.op(first.Covered(), second.Covered(), SkRegion::kUnion_Op);
  return out;
}

}  // namespace

SkRegion DraggableRegionsToSkRegion(
    const std::vector<blink::mojom::DraggableRegionPtr>& regions) {
  if (regions.empty()) {
    return SkRegion();
  }
  return Build(regions).result;
}

}  // namespace electron
