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

void BrowserWindow::OverrideNSWindowContentView(
    InspectableWebContentsView* view) {
  // Make NativeWindow use a NSView as content view.
  static_cast<NativeWindowMac*>(window())->OverrideNSWindowContentView();
  // Add webview to contentView.
  NSView* webView = view->GetNativeView().GetNativeNSView();
  NSView* contentView =
      [window()->GetNativeWindow().GetNativeNSWindow() contentView];
  [webView setFrame:[contentView bounds]];

  // ensure that buttons view is floated to top of view hierarchy
  NSArray* subviews = [contentView subviews];
  NSView* last = subviews.lastObject;
  [contentView addSubview:webView positioned:NSWindowBelow relativeTo:last];

  [contentView viewDidMoveToWindow];
}

void BrowserWindow::UpdateDraggableRegions(
    const std::vector<mojom::DraggableRegionPtr>& regions) {
  if (window_->has_frame() || !web_contents())
    return;

  // All ControlRegionViews should be added as children of the WebContentsView,
  // because WebContentsView will be removed and re-added when entering and
  // leaving fullscreen mode.
  NSView* webView = web_contents()->GetNativeView().GetNativeNSView();
  NSInteger webViewWidth = NSWidth([webView bounds]);
  NSInteger webViewHeight = NSHeight([webView bounds]);

  if ([webView respondsToSelector:@selector(setMouseDownCanMoveWindow:)]) {
    [webView setMouseDownCanMoveWindow:YES];
  }

  // Remove all ControlRegionViews that are added last time.
  // Note that [webView subviews] returns the view's mutable internal array and
  // it should be copied to avoid mutating the original array while enumerating
  // it.
  base::scoped_nsobject<NSArray> subviews([[webView subviews] copy]);
  for (NSView* subview in subviews.get())
    if ([subview isKindOfClass:[ControlRegionView class]])
      [subview removeFromSuperview];

  // Draggable regions are implemented by having the whole web view draggable
  // and overlaying regions that are not draggable.
  if (&draggable_regions_ != &regions)
    draggable_regions_ = mojo::Clone(regions);

  std::vector<gfx::Rect> drag_exclude_rects;
  if (regions.empty()) {
    drag_exclude_rects.emplace_back(0, 0, webViewWidth, webViewHeight);
  } else {
    drag_exclude_rects = CalculateNonDraggableRegions(
        DraggableRegionsToSkRegion(regions), webViewWidth, webViewHeight);
  }

  // Draggable regions on BrowserViews are independent from those of
  // BrowserWindows, so if a BrowserView with different draggable regions than
  // the BrowserWindow it belongs to is superimposed on top of that window, the
  // draggable regions of the BrowserView take precedence over those of the
  // BrowserWindow.
  for (NativeBrowserView* view : window_->browser_views()) {
    view->UpdateDraggableRegions(view->GetDraggableRegions());
  }

  // Create and add a ControlRegionView for each region that needs to be
  // excluded from the dragging.
  for (const auto& rect : drag_exclude_rects) {
    base::scoped_nsobject<NSView> controlRegion(
        [[ControlRegionView alloc] initWithFrame:NSZeroRect]);
    [controlRegion setFrame:NSMakeRect(rect.x(), webViewHeight - rect.bottom(),
                                       rect.width(), rect.height())];
    [webView addSubview:controlRegion];
  }

  // AppKit will not update its cache of mouseDownCanMoveWindow unless something
  // changes. Previously we tried adding an NSView and removing it, but for some
  // reason it required reposting the mouse-down event, and didn't always work.
  // Calling the below seems to be an effective solution.
  [[webView window] setMovableByWindowBackground:NO];
  [[webView window] setMovableByWindowBackground:YES];
}

}  // namespace electron::api
