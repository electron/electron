// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NATIVE_BROWSER_VIEW_MAC_H_
#define ELECTRON_SHELL_BROWSER_NATIVE_BROWSER_VIEW_MAC_H_

#import <Cocoa/Cocoa.h>
#include <vector>

#include "base/mac/scoped_nsobject.h"
#include "shell/browser/native_browser_view.h"

namespace electron {

class NativeBrowserViewMac : public NativeBrowserView {
 public:
  explicit NativeBrowserViewMac(
      InspectableWebContents* inspectable_web_contents);
  ~NativeBrowserViewMac() override;

  void SetAutoResizeFlags(uint8_t flags) override;
  void SetBounds(const gfx::Rect& bounds) override;
  gfx::Rect GetBounds() override;
  void SetBackgroundColor(SkColor color) override;

  void UpdateDraggableRegions(
      const std::vector<mojom::DraggableRegionPtr>& regions) override;

  void UpdateDraggableRegions(
      const std::vector<gfx::Rect>& drag_exclude_rects) override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NATIVE_BROWSER_VIEW_MAC_H_
