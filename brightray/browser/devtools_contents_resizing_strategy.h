// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_DEVTOOLS_CONTENTS_RESIZING_STRATEGY_H_
#define BRIGHTRAY_BROWSER_DEVTOOLS_CONTENTS_RESIZING_STRATEGY_H_

#include "base/macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

// This class knows how to resize both DevTools and inspected WebContents
// inside a browser window hierarchy.
class DevToolsContentsResizingStrategy {
 public:
  DevToolsContentsResizingStrategy();
  explicit DevToolsContentsResizingStrategy(
      const gfx::Rect& bounds);

  void CopyFrom(const DevToolsContentsResizingStrategy& strategy);
  bool Equals(const DevToolsContentsResizingStrategy& strategy);

  const gfx::Rect& bounds() const { return bounds_; }
  bool hide_inspected_contents() const { return hide_inspected_contents_; }

 private:
  // Contents bounds. When non-empty, used instead of insets.
  gfx::Rect bounds_;

  // Determines whether inspected contents is visible.
  bool hide_inspected_contents_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsContentsResizingStrategy);
};

// Applies contents resizing strategy, producing bounds for devtools and
// page contents views. Generally, page contents view is placed atop of devtools
// inside a common parent view, which size should be passed in |container_size|.
// When unknown, providing empty rect as previous devtools and contents bounds
// is allowed.
void ApplyDevToolsContentsResizingStrategy(
    const DevToolsContentsResizingStrategy& strategy,
    const gfx::Size& container_size,
    gfx::Rect* new_devtools_bounds,
    gfx::Rect* new_contents_bounds);

#endif  // BRIGHTRAY_BROWSER_DEVTOOLS_CONTENTS_RESIZING_STRATEGY_H_
