// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_BROWSER_VIEW_H_
#define ATOM_BROWSER_NATIVE_BROWSER_VIEW_H_

#include <vector>

#include "atom/common/draggable_region.h"
#include "base/macros.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"

namespace brightray {
class InspectableWebContents;
class InspectableWebContentsView;
}  // namespace brightray

namespace gfx {
class Rect;
}

namespace atom {

enum AutoResizeFlags {
  kAutoResizeWidth = 0x1,
  kAutoResizeHeight = 0x2,
};

class NativeBrowserView {
 public:
  virtual ~NativeBrowserView();

  static NativeBrowserView* Create(
      brightray::InspectableWebContents* inspectable_web_contents);

  brightray::InspectableWebContents* GetInspectableWebContents() {
    return inspectable_web_contents_;
  }

  brightray::InspectableWebContentsView* GetInspectableWebContentsView();
  content::WebContents* GetWebContents();

  virtual void SetAutoResizeFlags(uint8_t flags) = 0;
  virtual void SetBounds(const gfx::Rect& bounds) = 0;
  virtual void SetBackgroundColor(SkColor color) = 0;

  // Called when the window needs to update its draggable region.
  virtual void UpdateDraggableRegions(
      const std::vector<gfx::Rect>& system_drag_exclude_areas) {}

 protected:
  explicit NativeBrowserView(
      brightray::InspectableWebContents* inspectable_web_contents);

  brightray::InspectableWebContents* inspectable_web_contents_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NativeBrowserView);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_BROWSER_VIEW_H_
