// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_DRAGGABLE_REGION_H_
#define ATOM_COMMON_DRAGGABLE_REGION_H_

#include "ui/gfx/geometry/rect.h"

namespace atom {

struct DraggableRegion {
  bool draggable;
  gfx::Rect bounds;

  DraggableRegion();
};

}  // namespace atom

#endif  // ATOM_COMMON_DRAGGABLE_REGION_H_
