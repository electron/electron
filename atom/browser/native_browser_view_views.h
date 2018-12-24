// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_BROWSER_VIEW_VIEWS_H_
#define ATOM_BROWSER_NATIVE_BROWSER_VIEW_VIEWS_H_

#include "atom/browser/native_browser_view.h"

namespace atom {

class NativeBrowserViewViews : public NativeBrowserView {
 public:
  explicit NativeBrowserViewViews(
      InspectableWebContents* inspectable_web_contents);
  ~NativeBrowserViewViews() override;

  uint8_t GetAutoResizeFlags() { return auto_resize_flags_; }
  void SetAutoResizeFlags(uint8_t flags) override;
  void SetAutoResizeProportions(gfx::Size window_size);
  void AutoResize(const gfx::Rect& new_window,
                  int width_delta,
                  int height_delta);
  void SetBounds(const gfx::Rect& bounds) override;
  void SetBackgroundColor(SkColor color) override;

 private:
  uint8_t auto_resize_flags_;

  bool auto_horizontal_proportion_set_;
  float auto_horizontal_proportion_width_;
  float auto_horizontal_proportion_left_;
  bool auto_vertical_proportion_set_;
  float auto_vertical_proportion_height_;
  float auto_vertical_proportion_top_;
  void ResetAutoResizeProportions();

  DISALLOW_COPY_AND_ASSIGN(NativeBrowserViewViews);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_BROWSER_VIEW_VIEWS_H_
