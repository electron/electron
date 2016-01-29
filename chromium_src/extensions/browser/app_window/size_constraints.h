// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_APP_WINDOW_SIZE_CONSTRAINTS_H_
#define EXTENSIONS_BROWSER_APP_WINDOW_SIZE_CONSTRAINTS_H_

#include "ui/gfx/geometry/size.h"

namespace gfx {
class Insets;
}

namespace extensions {

class SizeConstraints {
 public:
  // The value SizeConstraints uses to represent an unbounded width or height.
  // This is an enum so that it can be declared inline here.
  enum { kUnboundedSize = 0 };

  SizeConstraints();
  SizeConstraints(const gfx::Size& min_size, const gfx::Size& max_size);
  ~SizeConstraints();

  // Adds frame insets to a size constraint.
  static gfx::Size AddFrameToConstraints(const gfx::Size& size_constraints,
                                         const gfx::Insets& frame_insets);

  // Returns the bounds with its size clamped to the min/max size.
  gfx::Size ClampSize(gfx::Size size) const;

  // When gfx::Size is used as a min/max size, a zero represents an unbounded
  // component. This method checks whether either component is specified.
  // Note we can't use gfx::Size::IsEmpty as it returns true if either width
  // or height is zero.
  bool HasMinimumSize() const;
  bool HasMaximumSize() const;

  // This returns true if all components are specified, and min and max are
  // equal.
  bool HasFixedSize() const;

  gfx::Size GetMaximumSize() const;
  gfx::Size GetMinimumSize() const;

  void set_minimum_size(const gfx::Size& min_size);
  void set_maximum_size(const gfx::Size& max_size);

 private:
  gfx::Size minimum_size_;
  gfx::Size maximum_size_;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_APP_WINDOW_SIZE_CONSTRAINTS_H_
