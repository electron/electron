// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_COMMON_DRAGGABLE_REGION_H_
#define ELECTRON_COMMON_DRAGGABLE_REGION_H_

#include "ui/gfx/geometry/rect.h"

namespace electron {

struct DraggableRegion {
  bool draggable;
  gfx::Rect bounds;

  DraggableRegion();
};

}  // namespace electron

#endif  // ELECTRON_COMMON_DRAGGABLE_REGION_H_
