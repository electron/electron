// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_browser_view_mac.h"

#include "brightray/browser/inspectable_web_contents_view.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/geometry/rect.h"

using base::scoped_nsobject;

// Match view::Views behavior where the view sticks to the top-left origin.
const NSAutoresizingMaskOptions kDefaultAutoResizingMask =
    NSViewMaxXMargin | NSViewMinYMargin;

@interface DragRegionView : NSView

@property (assign) NSPoint initialLocation;

@end

@implementation DragRegionView

- (BOOL)mouseDownCanMoveWindow
{
  return NO;
}

- (void)mouseDown:(NSEvent *)event
{
  if ([self.window respondsToSelector:@selector(performWindowDragWithEvent:)]) {
    [self.window performWindowDragWithEvent:event];
    return;
  }

  self.initialLocation = [event locationInWindow];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
  if ([self.window respondsToSelector:@selector(performWindowDragWithEvent:)]) {
    return;
  }

  NSPoint currentLocation;
  NSPoint newOrigin;

  NSRect screenFrame = [[NSScreen mainScreen] frame];
  NSRect windowFrame = [self.window frame];

  currentLocation = [NSEvent mouseLocation];
  newOrigin.x = currentLocation.x - self.initialLocation.x;
  newOrigin.y = currentLocation.y - self.initialLocation.y;

  // Don't let window get dragged up under the menu bar
  if( (newOrigin.y+windowFrame.size.height) > (screenFrame.origin.y+screenFrame.size.height) ){
    newOrigin.y=screenFrame.origin.y + (screenFrame.size.height-windowFrame.size.height);
  }

  // Move the window to the new location
  [self.window setFrameOrigin:newOrigin];
}

// Debugging tips:
// Uncomment the following four lines to color DragRegionViews bright red
- (void)drawRect:(NSRect)aRect
{
    [[NSColor redColor] set];
    NSRectFill([self bounds]);
}

@end

namespace atom {

NativeBrowserViewMac::NativeBrowserViewMac(
    brightray::InspectableWebContentsView* web_contents_view)
    : NativeBrowserView(web_contents_view) {
  auto* view = GetInspectableWebContentsView()->GetNativeView();
  view.autoresizingMask = kDefaultAutoResizingMask;
}

NativeBrowserViewMac::~NativeBrowserViewMac() {}

void NativeBrowserViewMac::SetAutoResizeFlags(uint8_t flags) {
  NSAutoresizingMaskOptions autoresizing_mask = kDefaultAutoResizingMask;
  if (flags & kAutoResizeWidth) {
    autoresizing_mask |= NSViewWidthSizable;
  }
  if (flags & kAutoResizeHeight) {
    autoresizing_mask |= NSViewHeightSizable;
  }

  auto* view = GetInspectableWebContentsView()->GetNativeView();
  view.autoresizingMask = autoresizing_mask;
}

void NativeBrowserViewMac::SetBounds(const gfx::Rect& bounds) {
  auto* view = GetInspectableWebContentsView()->GetNativeView();
  auto* superview = view.superview;
  const auto superview_height = superview ? superview.frame.size.height : 0;
  view.frame =
      NSMakeRect(bounds.x(), superview_height - bounds.y() - bounds.height(),
                 bounds.width(), bounds.height());
}

void NativeBrowserViewMac::SetBackgroundColor(SkColor color) {
  auto* view = GetInspectableWebContentsView()->GetNativeView();
  view.wantsLayer = YES;
  view.layer.backgroundColor = skia::CGColorCreateFromSkColor(color);
}

void NativeBrowserViewMac::UpdateDraggableRegions(
    const std::vector<DraggableRegion>& regions) {
  NSView* webView = GetInspectableWebContentsView()->GetNativeView();
  NSInteger webViewHeight = NSHeight([webView bounds]);

  // Remove all DraggableRegionViews that are added last time.
  // Note that [webView subviews] returns the view's mutable internal array and
  // it should be copied to avoid mutating the original array while enumerating
  // it.
  base::scoped_nsobject<NSArray> subviews([[webView subviews] copy]);
  for (NSView* subview in subviews.get())
    if ([subview isKindOfClass:[DragRegionView class]])
      [subview removeFromSuperview];

  for (const DraggableRegion& region : regions) {
    base::scoped_nsobject<NSView> dragRegion(
        [[DragRegionView alloc] initWithFrame:NSZeroRect]);
    [dragRegion setFrame:NSMakeRect(region.bounds.x(),
                                    webViewHeight - region.bounds.bottom(),
                                    region.bounds.width(),
                                    region.bounds.height())];
    [webView addSubview:dragRegion];
  }
}

// static
NativeBrowserView* NativeBrowserView::Create(
    brightray::InspectableWebContentsView* web_contents_view) {
  return new NativeBrowserViewMac(web_contents_view);
}

}  // namespace atom
