// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brightray/browser/devtools_contents_resizing_strategy.h"

#include <algorithm>

DevToolsContentsResizingStrategy::DevToolsContentsResizingStrategy()
    : hide_inspected_contents_(false) {
}

DevToolsContentsResizingStrategy::DevToolsContentsResizingStrategy(
    const gfx::Rect& bounds)
    : bounds_(bounds),
      hide_inspected_contents_(bounds_.IsEmpty() && !bounds_.x() &&
          !bounds_.y()) {
}

void DevToolsContentsResizingStrategy::CopyFrom(
    const DevToolsContentsResizingStrategy& strategy) {
  bounds_ = strategy.bounds();
  hide_inspected_contents_ = strategy.hide_inspected_contents();
}

bool DevToolsContentsResizingStrategy::Equals(
    const DevToolsContentsResizingStrategy& strategy) {
  return bounds_ == strategy.bounds() &&
      hide_inspected_contents_ == strategy.hide_inspected_contents();
}

void ApplyDevToolsContentsResizingStrategy(
    const DevToolsContentsResizingStrategy& strategy,
    const gfx::Size& container_size,
    gfx::Rect* new_devtools_bounds,
    gfx::Rect* new_contents_bounds) {
  new_devtools_bounds->SetRect(
      0, 0, container_size.width(), container_size.height());

  const gfx::Rect& bounds = strategy.bounds();
  if (bounds.size().IsEmpty() && !strategy.hide_inspected_contents()) {
    new_contents_bounds->SetRect(
        0, 0, container_size.width(), container_size.height());
    return;
  }

  int left = std::min(bounds.x(), container_size.width());
  int top = std::min(bounds.y(), container_size.height());
  int width = std::min(bounds.width(), container_size.width() - left);
  int height = std::min(bounds.height(), container_size.height() - top);
  new_contents_bounds->SetRect(left, top, width, height);
}
