// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_browser_window.h"

#import <Cocoa/Cocoa.h>

@interface NSView (WebContentsView)
- (void)setMouseDownCanMoveWindow:(BOOL)can_move;
@end

namespace atom {

namespace api {

void BrowserWindow::UpdateDraggableRegions(
    content::RenderFrameHost* rfh,
    const std::vector<DraggableRegion>& regions) {
  if (window_->has_frame())
    return;
  NSView* webView = web_contents()->GetNativeView();
  if ([webView respondsToSelector:@selector(setMouseDownCanMoveWindow:)]) {
    [webView setMouseDownCanMoveWindow:YES];
  }
  window_->UpdateDraggableRegions(regions);
}

}  // namespace api

}  // namespace atom
