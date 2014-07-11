// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_DEVTOOLS_CONTENTS_RESIZING_STRATEGY_H_
#define BRIGHTRAY_BROWSER_DEVTOOLS_CONTENTS_RESIZING_STRATEGY_H_

#include "base/basictypes.h"
#include "ui/gfx/insets.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

// This class knows how to resize both DevTools and inspected WebContents
// inside a browser window hierarchy.
class DevToolsContentsResizingStrategy {
 public:
  DevToolsContentsResizingStrategy();
  DevToolsContentsResizingStrategy(
      const gfx::Insets& insets,
      const gfx::Size& min_size);
  explicit DevToolsContentsResizingStrategy(const gfx::Rect& bounds);

  void CopyFrom(const DevToolsContentsResizingStrategy& strategy);
  bool Equals(const DevToolsContentsResizingStrategy& strategy);

  const gfx::Insets& insets() const { return insets_; }
  const gfx::Size& min_size() const { return min_size_; }
  const gfx::Rect& bounds() const { return bounds_; }

 private:
  // Insets of contents inside DevTools.
  gfx::Insets insets_;

  // Minimum size of contents.
  gfx::Size min_size_;

  // Contents bounds. When non-empty, used instead of insets.
  gfx::Rect bounds_;

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
    const gfx::Rect& old_devtools_bounds,
    const gfx::Rect& old_contents_bounds,
    gfx::Rect* new_devtools_bounds,
    gfx::Rect* new_contents_bounds);

#endif  // BRIGHTRAY_BROWSER_DEVTOOLS_CONTENTS_RESIZING_STRATEGY_H_
