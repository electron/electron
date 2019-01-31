// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_browser_view_mac.h"

#include "atom/browser/ui/inspectable_web_contents.h"
#include "atom/browser/ui/inspectable_web_contents_view.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/geometry/rect.h"

// Match view::Views behavior where the view sticks to the top-left origin.
const NSAutoresizingMaskOptions kDefaultAutoResizingMask =
    NSViewMaxXMargin | NSViewMinYMargin;

@interface DragRegionView : NSView

@property(assign) NSPoint initialLocation;

@end

@interface NSWindow ()
- (void)performWindowDragWithEvent:(NSEvent*)event;
@end

@implementation DragRegionView

@synthesize initialLocation;

- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

- (NSView*)hitTest:(NSPoint)aPoint {
  // Pass-through events that don't hit one of the exclusion zones
  for (NSView* exlusion_zones in [self subviews]) {
    if ([exlusion_zones hitTest:aPoint])
      return nil;
  }

  return self;
}

- (void)mouseDown:(NSEvent*)event {
  if ([self.window respondsToSelector:@selector(performWindowDragWithEvent)]) {
    // According to Google, using performWindowDragWithEvent:
    // does not generate a NSWindowWillMoveNotification. Hence post one.
    [[NSNotificationCenter defaultCenter]
        postNotificationName:NSWindowWillMoveNotification
                      object:self];

    if (@available(macOS 10.11, *)) {
      [self.window performWindowDragWithEvent:event];
    }
    return;
  }

  if (self.window.styleMask & NSWindowStyleMaskFullScreen) {
    return;
  }

  self.initialLocation = [event locationInWindow];
}

- (void)mouseDragged:(NSEvent*)theEvent {
  if ([self.window respondsToSelector:@selector(performWindowDragWithEvent)]) {
    return;
  }

  if (self.window.styleMask & NSWindowStyleMaskFullScreen) {
    return;
  }

  NSPoint currentLocation = [NSEvent mouseLocation];
  NSPoint newOrigin;

  NSRect screenFrame = [[NSScreen mainScreen] frame];
  NSSize screenSize = screenFrame.size;
  NSRect windowFrame = [self.window frame];
  NSSize windowSize = windowFrame.size;

  newOrigin.x = currentLocation.x - self.initialLocation.x;
  newOrigin.y = currentLocation.y - self.initialLocation.y;

  BOOL inMenuBar = (newOrigin.y + windowSize.height) >
                   (screenFrame.origin.y + screenSize.height);
  BOOL screenAboveMainScreen = false;

  if (inMenuBar) {
    for (NSScreen* screen in [NSScreen screens]) {
      NSRect currentScreenFrame = [screen frame];
      BOOL isHigher = currentScreenFrame.origin.y > screenFrame.origin.y;

      // If there's another screen that is generally above the current screen,
      // we'll draw a new rectangle that is just above the current screen. If
      // the "higher" screen intersects with this rectangle, we'll allow drawing
      // above the menubar.
      if (isHigher) {
        NSRect aboveScreenRect =
            NSMakeRect(screenFrame.origin.x,
                       screenFrame.origin.y + screenFrame.size.height - 10,
                       screenFrame.size.width, 200);

        BOOL screenAboveIntersects =
            NSIntersectsRect(currentScreenFrame, aboveScreenRect);

        if (screenAboveIntersects) {
          screenAboveMainScreen = true;
          break;
        }
      }
    }
  }

  // Don't let window get dragged up under the menu bar
  if (inMenuBar && !screenAboveMainScreen) {
    newOrigin.y = screenFrame.origin.y +
                  (screenFrame.size.height - windowFrame.size.height);
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
    InspectableWebContents* inspectable_web_contents)
    : NativeBrowserView(inspectable_web_contents) {
  auto* view =
      GetInspectableWebContentsView()->GetNativeView().GetNativeNSView();
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
  if (flags & kAutoResizeHorizontal) {
    autoresizing_mask |=
        NSViewMaxXMargin | NSViewMinXMargin | NSViewWidthSizable;
  }
  if (flags & kAutoResizeVertical) {
    autoresizing_mask |=
        NSViewMaxYMargin | NSViewMinYMargin | NSViewHeightSizable;
  }

  auto* view =
      GetInspectableWebContentsView()->GetNativeView().GetNativeNSView();
  view.autoresizingMask = autoresizing_mask;
}

void NativeBrowserViewMac::SetBounds(const gfx::Rect& bounds) {
  auto* view =
      GetInspectableWebContentsView()->GetNativeView().GetNativeNSView();
  auto* superview = view.superview;
  const auto superview_height = superview ? superview.frame.size.height : 0;
  view.frame =
      NSMakeRect(bounds.x(), superview_height - bounds.y() - bounds.height(),
                 bounds.width(), bounds.height());
}

void NativeBrowserViewMac::SetBackgroundColor(SkColor color) {
  auto* view =
      GetInspectableWebContentsView()->GetNativeView().GetNativeNSView();
  view.wantsLayer = YES;
  view.layer.backgroundColor = skia::CGColorCreateFromSkColor(color);
}

void NativeBrowserViewMac::UpdateDraggableRegions(
    const std::vector<gfx::Rect>& drag_exclude_rects) {
  NSView* web_view = GetWebContents()->GetNativeView().GetNativeNSView();
  NSView* inspectable_view =
      GetInspectableWebContentsView()->GetNativeView().GetNativeNSView();
  NSView* window_content_view = inspectable_view.superview;
  const auto window_content_view_height = NSHeight(window_content_view.bounds);

  // Remove all DragRegionViews that were added last time. Note that we need
  // to copy the `subviews` array to avoid mutation during iteration.
  base::scoped_nsobject<NSArray> subviews([[web_view subviews] copy]);
  for (NSView* subview in subviews.get()) {
    if ([subview isKindOfClass:[DragRegionView class]]) {
      [subview removeFromSuperview];
    }
  }

  // Create one giant NSView that is draggable.
  base::scoped_nsobject<NSView> drag_region_view(
      [[DragRegionView alloc] initWithFrame:web_view.bounds]);
  [web_view addSubview:drag_region_view];

  // Then, on top of that, add "exclusion zones"
  for (const auto& rect : drag_exclude_rects) {
    const auto window_content_view_exclude_rect =
        NSMakeRect(rect.x(), window_content_view_height - rect.bottom(),
                   rect.width(), rect.height());
    const auto drag_region_view_exclude_rect =
        [window_content_view convertRect:window_content_view_exclude_rect
                                  toView:drag_region_view];

    base::scoped_nsobject<NSView> exclude_drag_region_view(
        [[ExcludeDragRegionView alloc]
            initWithFrame:drag_region_view_exclude_rect]);
    [drag_region_view addSubview:exclude_drag_region_view];
  }
}

// static
NativeBrowserView* NativeBrowserView::Create(
    InspectableWebContents* inspectable_web_contents) {
  return new NativeBrowserViewMac(inspectable_web_contents);
}

}  // namespace atom
