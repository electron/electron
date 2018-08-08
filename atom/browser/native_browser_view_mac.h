// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_BROWSER_VIEW_MAC_H_
#define ATOM_BROWSER_NATIVE_BROWSER_VIEW_MAC_H_

#import <Cocoa/Cocoa.h>
#include <vector>

#include "atom/browser/native_browser_view.h"
#include "atom/common/draggable_region.h"
#include "base/mac/scoped_nsobject.h"

namespace atom {

class NativeBrowserViewMac : public NativeBrowserView {
 public:
  explicit NativeBrowserViewMac(
      brightray::InspectableWebContents* inspectable_web_contents);
  ~NativeBrowserViewMac() override;

  void SetAutoResizeFlags(uint8_t flags) override;
  void SetBounds(const gfx::Rect& bounds) override;
  void SetBackgroundColor(SkColor color) override;

  void UpdateDraggableRegions(
      const std::vector<gfx::Rect>& system_drag_exclude_areas) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NativeBrowserViewMac);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_BROWSER_VIEW_MAC_H_
