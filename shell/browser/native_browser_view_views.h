// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NATIVE_BROWSER_VIEW_VIEWS_H_
#define ELECTRON_SHELL_BROWSER_NATIVE_BROWSER_VIEW_VIEWS_H_

#include <memory>
#include <vector>

#include "shell/browser/native_browser_view.h"
#include "third_party/skia/include/core/SkRegion.h"

namespace electron {

class NativeBrowserViewViews : public NativeBrowserView {
 public:
  explicit NativeBrowserViewViews(
      InspectableWebContents* inspectable_web_contents);
  ~NativeBrowserViewViews() override;

  void SetAutoResizeProportions(const gfx::Size& window_size);
  void AutoResize(const gfx::Rect& new_window,
                  int width_delta,
                  int height_delta);
  uint8_t GetAutoResizeFlags() { return auto_resize_flags_; }

  // NativeBrowserView:
  void SetAutoResizeFlags(uint8_t flags) override;
  void SetBounds(const gfx::Rect& bounds) override;
  gfx::Rect GetBounds() override;
  void SetBackgroundColor(SkColor color) override;
  void UpdateDraggableRegions(
      const std::vector<mojom::DraggableRegionPtr>& regions) override;

  // WebContentsObserver:
  void RenderViewReady() override;

  SkRegion* draggable_region() const { return draggable_region_.get(); }

 private:
  void ResetAutoResizeProportions();

  uint8_t auto_resize_flags_ = 0;

  bool auto_horizontal_proportion_set_ = false;
  float auto_horizontal_proportion_width_ = 0.;
  float auto_horizontal_proportion_left_ = 0.;

  bool auto_vertical_proportion_set_ = false;
  float auto_vertical_proportion_height_ = 0.;
  float auto_vertical_proportion_top_ = 0.;

  std::unique_ptr<SkRegion> draggable_region_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NATIVE_BROWSER_VIEW_VIEWS_H_
