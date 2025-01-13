// Copyright (c) 2024 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_API_ELECTRON_SMOOTH_ROUND_RECT_H_
#define ELECTRON_SHELL_RENDERER_API_ELECTRON_SMOOTH_ROUND_RECT_H_

#include "third_party/skia/include/core/SkPath.h"

namespace electron {

SkPath DrawSmoothRoundRect(float x,
                           float y,
                           float width,
                           float height,
                           float roundness,
                           float top_left_radius,
                           float top_right_radius,
                           float bottom_right_radius,
                           float bottom_left_radius);

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_API_ELECTRON_SMOOTH_ROUND_RECT_H_
