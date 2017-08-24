// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_browser_view_mac.h"

#include "brightray/browser/inspectable_web_contents_view.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/geometry/rect.h"

// Match view::Views behavior where the view sticks to the top-left origin.
const NSAutoresizingMaskOptions kDefaultAutoResizingMask =
    NSViewMaxXMargin | NSViewMinYMargin;

@interface DragRegionView : NSView

@property (assign) NSPoint initialLocation;

@end

@interface NSWindow ()
- (void)performWindowDragWithEvent:(NSEvent *)event;
@end

@implementation DragRegionView

- (BOOL)mouseDownCanMoveWindow
{
  return NO;
}

- (NSView *)hitTest:(NSPoint)aPoint
{
    // Pass-through events that don't hit one of the exclusion zones
    for (NSView *exlusion_zones in [self subviews]) {
      if ([exlusion_zones hitTest:aPoint])
        return nil;
    }

    return self;
}

- (void)mouseDown:(NSEvent *)event
{
  if ([self.window respondsToSelector:@selector(performWindowDragWithEvent)]) {
    [self.window performWindowDragWithEvent:event];
    return;
  }

  self.initialLocation = [event locationInWindow];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
  if ([self.window respondsToSelector:@selector(performWindowDragWithEvent)]) {
    return;
  }

  NSPoint currentLocation = [NSEvent mouseLocation];
  NSPoint newOrigin;

  NSRect screenFrame = [[NSScreen mainScreen] frame];
  NSRect windowFrame = [self.window frame];

  newOrigin.x = currentLocation.x - self.initialLocation.x;
  newOrigin.y = currentLocation.y - self.initialLocation.y;

  // Don't let window get dragged up under the menu bar
  if ((newOrigin.y + windowFrame.size.height) > (screenFrame.origin.y + screenFrame.size.height)) {
    newOrigin.y = screenFrame.origin.y + (screenFrame.size.height - windowFrame.size.height);
  }

  // Move the window to the new location
  [self.window setFrameOrigin:newOrigin];
}

// Debugging tips:
// Uncomment the following four lines to color DragRegionView bright red
// #ifdef DEBUG_DRAG_REGIONS
// - (void)drawRect:(NSRect)aRect
// {
//     [[NSColor redColor] set];
//     NSRectFill([self bounds]);
// }
// #endif

@end

@interface ExcludeDragRegionView : NSView
@end

@implementation ExcludeDragRegionView

- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

// Debugging tips:
// Uncomment the following four lines to color ExcludeDragRegionView bright red
// #ifdef DEBUG_DRAG_REGIONS
// - (void)drawRect:(NSRect)aRect
// {
//     [[NSColor greenColor] set];
//     NSRectFill([self bounds]);
// }
// #endif

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
    const std::vector<gfx::Rect>& system_drag_exclude_areas) {
  NSView* webView = GetInspectableWebContentsView()->GetNativeView();

  NSInteger superViewHeight = NSHeight([webView.superview bounds]);
  NSInteger webViewHeight = NSHeight([webView bounds]);
  NSInteger webViewWidth = NSWidth([webView bounds]);
  NSInteger webViewX = NSMinX([webView frame]);
  NSInteger webViewY = 0;

  // Apple's NSViews have their coordinate system originate at the bottom left,
  // meaning that we need to be a bit smarter when it comes to calculating our
  // current top offset
  if (webViewHeight > superViewHeight) {
    webViewY = std::abs(webViewHeight - superViewHeight - (std::abs(NSMinY([webView frame]))));
  } else {
    webViewY = superViewHeight - NSMaxY([webView frame]);
  }

  // Remove all DraggableRegionViews that are added last time.
  // Note that [webView subviews] returns the view's mutable internal array and
  // it should be copied to avoid mutating the original array while enumerating
  // it.
  base::scoped_nsobject<NSArray> subviews([[webView subviews] copy]);
  for (NSView* subview in subviews.get())
    if ([subview isKindOfClass:[DragRegionView class]])
      [subview removeFromSuperview];

  // Create one giant NSView that is draggable.
  base::scoped_nsobject<NSView> dragRegion(
        [[DragRegionView alloc] initWithFrame:NSZeroRect]);
    [dragRegion setFrame:NSMakeRect(0,
                                    0,
                                    webViewWidth,
                                    webViewHeight)];

  // Then, on top of that, add "exclusion zones"
  for (auto iter = system_drag_exclude_areas.begin();
       iter != system_drag_exclude_areas.end();
       ++iter) {
    base::scoped_nsobject<NSView> controlRegion(
        [[ExcludeDragRegionView alloc] initWithFrame:NSZeroRect]);
    [controlRegion setFrame:NSMakeRect(iter->x() - webViewX,
                                       webViewHeight - iter->bottom() + webViewY,
                                       iter->width(),
                                       iter->height())];
    [dragRegion addSubview:controlRegion];
  }

  // Add the DragRegion to the WebView
  [webView addSubview:dragRegion];
}

// static
NativeBrowserView* NativeBrowserView::Create(
    brightray::InspectableWebContentsView* web_contents_view) {
  return new NativeBrowserViewMac(web_contents_view);
}

}  // namespace atom
