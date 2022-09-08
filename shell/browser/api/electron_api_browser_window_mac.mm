// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_browser_window.h"

#include <memory>
#include <vector>

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "shell/browser/native_browser_view.h"
#include "shell/browser/native_window_mac.h"
#include "shell/browser/ui/cocoa/electron_inspectable_web_contents_view.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"

namespace electron::api {

void BrowserWindow::UpdateDraggableRegions(
    const std::vector<mojom::DraggableRegionPtr>& regions) {
  if (&draggable_regions_ != &regions && web_contents()) {
    draggable_regions_ = mojo::Clone(regions);
  }

  static_cast<NativeWindowMac*>(window_.get())
      ->UpdateDraggableRegions(draggable_regions_);
}

}  // namespace electron::api
