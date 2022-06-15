// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/native_browser_view_mac.h"

#import <objc/runtime.h>
#include <vector>

#include "shell/browser/ui/drag_util.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
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

+ (void)load {
  if (getenv("ELECTRON_DEBUG_DRAG_REGIONS")) {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      SEL originalSelector = @selector(drawRect:);
      SEL swizzledSelector = @selector(drawDebugRect:);

      Method originalMethod =
          class_getInstanceMethod([self class], originalSelector);
      Method swizzledMethod =
          class_getInstanceMethod([self class], swizzledSelector);
      BOOL didAddMethod =
          class_addMethod([self class], originalSelector,
                          method_getImplementation(swizzledMethod),
                          method_getTypeEncoding(swizzledMethod));

      if (didAddMethod) {
        class_replaceMethod([self class], swizzledSelector,
                            method_getImplementation(originalMethod),
                            method_getTypeEncoding(originalMethod));
      } else {
        method_exchangeImplementations(originalMethod, swizzledMethod);
      }
    });
  }
}

- (BOOL)mouseDownCanMoveWindow {
  return
      [self.window respondsToSelector:@selector(performWindowDragWithEvent:)];
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
  return YES;
}

- (BOOL)shouldIgnoreMouseEvent {
  NSEventType type = [[NSApp currentEvent] type];
  return type != NSEventTypeLeftMouseDragged &&
         type != NSEventTypeLeftMouseDown;
}

- (NSView*)hitTest:(NSPoint)point {
  // Pass-through events that hit one of the exclusion zones
  for (NSView* exclusion_zones in [self subviews]) {
    if ([exclusion_zones hitTest:point])
      return nil;
  }

  return self;
}

- (void)mouseDown:(NSEvent*)event {
  [super mouseDown:event];

  if ([self.window respondsToSelector:@selector(performWindowDragWithEvent:)]) {
    // According to Google, using performWindowDragWithEvent:
    // does not generate a NSWindowWillMoveNotification. Hence post one.
    [[NSNotificationCenter defaultCenter]
        postNotificationName:NSWindowWillMoveNotification
                      object:self];

    [self.window performWindowDragWithEvent:event];

    return;
  }

  if (self.window.styleMask & NSWindowStyleMaskFullScreen) {
    return;
  }

  self.initialLocation = [event locationInWindow];
}

- (void)mouseDragged:(NSEvent*)event {
  if ([self.window respondsToSelector:@selector(performWindowDragWithEvent:)]) {
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

// For debugging purposes only.
- (void)drawDebugRect:(NSRect)aRect {
  [[[NSColor greenColor] colorWithAlphaComponent:0.5] set];
  NSRectFill([self bounds]);
}

@end

@interface ExcludeDragRegionView : NSView
@end

@implementation ExcludeDragRegionView

+ (void)load {
  if (getenv("ELECTRON_DEBUG_DRAG_REGIONS")) {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      SEL originalSelector = @selector(drawRect:);
      SEL swizzledSelector = @selector(drawDebugRect:);

      Method originalMethod =
          class_getInstanceMethod([self class], originalSelector);
      Method swizzledMethod =
          class_getInstanceMethod([self class], swizzledSelector);
      BOOL didAddMethod =
          class_addMethod([self class], originalSelector,
                          method_getImplementation(swizzledMethod),
                          method_getTypeEncoding(swizzledMethod));

      if (didAddMethod) {
        class_replaceMethod([self class], swizzledSelector,
                            method_getImplementation(originalMethod),
                            method_getTypeEncoding(originalMethod));
      } else {
        method_exchangeImplementations(originalMethod, swizzledMethod);
      }
    });
  }
}

- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

// For debugging purposes only.
- (void)drawDebugRect:(NSRect)aRect {
  [[[NSColor redColor] colorWithAlphaComponent:0.5] set];
  NSRectFill([self bounds]);
}

@end

namespace electron {

NativeBrowserViewMac::NativeBrowserViewMac(
    InspectableWebContents* inspectable_web_contents)
    : NativeBrowserView(inspectable_web_contents) {
  auto* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return;
  auto* view = iwc_view->GetNativeView().GetNativeNSView();
  view.autoresizingMask = kDefaultAutoResizingMask;
}

NativeBrowserViewMac::~NativeBrowserViewMac() = default;

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

  auto* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return;
  auto* view = iwc_view->GetNativeView().GetNativeNSView();
  view.autoresizingMask = autoresizing_mask;
}

void NativeBrowserViewMac::SetBounds(const gfx::Rect& bounds) {
  auto* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return;
  auto* view = iwc_view->GetNativeView().GetNativeNSView();
  auto* superview = view.superview;
  const auto superview_height = superview ? superview.frame.size.height : 0;
  view.frame =
      NSMakeRect(bounds.x(), superview_height - bounds.y() - bounds.height(),
                 bounds.width(), bounds.height());

  // Ensure draggable regions are properly updated to reflect new bounds.
  UpdateDraggableRegions(draggable_regions_);
}

gfx::Rect NativeBrowserViewMac::GetBounds() {
  auto* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return gfx::Rect();
  NSView* view = iwc_view->GetNativeView().GetNativeNSView();
  const int superview_height =
      (view.superview) ? view.superview.frame.size.height : 0;
  return gfx::Rect(
      view.frame.origin.x,
      superview_height - view.frame.origin.y - view.frame.size.height,
      view.frame.size.width, view.frame.size.height);
}

void NativeBrowserViewMac::SetBackgroundColor(SkColor color) {
  auto* iwc_view = GetInspectableWebContentsView();
  if (!iwc_view)
    return;
  auto* view = iwc_view->GetNativeView().GetNativeNSView();
  view.wantsLayer = YES;
  view.layer.backgroundColor = skia::CGColorCreateFromSkColor(color);
}

void NativeBrowserViewMac::UpdateDraggableRegions(
    const std::vector<gfx::Rect>& drag_exclude_rects) {
  if (!inspectable_web_contents_)
    return;
  auto* web_contents = inspectable_web_contents_->GetWebContents();
  auto* iwc_view = GetInspectableWebContentsView();
  NSView* web_view = web_contents->GetNativeView().GetNativeNSView();
  NSView* inspectable_view = iwc_view->GetNativeView().GetNativeNSView();
  NSView* window_content_view = inspectable_view.superview;

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

  // Then, on top of that, add "exclusion zones".
  auto const offset = GetBounds().OffsetFromOrigin();
  const auto window_content_view_height = NSHeight(window_content_view.bounds);
  for (const auto& rect : drag_exclude_rects) {
    const auto x = rect.x() + offset.x();
    const auto y = window_content_view_height - (rect.bottom() + offset.y());
    const auto exclude_rect = NSMakeRect(x, y, rect.width(), rect.height());

    const auto drag_region_view_exclude_rect =
        [window_content_view convertRect:exclude_rect toView:drag_region_view];

    base::scoped_nsobject<NSView> exclude_drag_region_view(
        [[ExcludeDragRegionView alloc]
            initWithFrame:drag_region_view_exclude_rect]);
    [drag_region_view addSubview:exclude_drag_region_view];
  }
}

void NativeBrowserViewMac::UpdateDraggableRegions(
    const std::vector<mojom::DraggableRegionPtr>& regions) {
  if (!inspectable_web_contents_)
    return;
  auto* web_contents = inspectable_web_contents_->GetWebContents();
  NSView* web_view = web_contents->GetNativeView().GetNativeNSView();

  NSInteger webViewWidth = NSWidth([web_view bounds]);
  NSInteger webViewHeight = NSHeight([web_view bounds]);

  // Draggable regions are implemented by having the whole web view draggable
  // and overlaying regions that are not draggable.
  if (&draggable_regions_ != &regions)
    draggable_regions_ = mojo::Clone(regions);

  std::vector<gfx::Rect> drag_exclude_rects;
  if (draggable_regions_.empty()) {
    drag_exclude_rects.emplace_back(0, 0, webViewWidth, webViewHeight);
  } else {
    drag_exclude_rects = CalculateNonDraggableRegions(
        DraggableRegionsToSkRegion(draggable_regions_), webViewWidth,
        webViewHeight);
  }

  UpdateDraggableRegions(drag_exclude_rects);
}

// static
NativeBrowserView* NativeBrowserView::Create(
    InspectableWebContents* inspectable_web_contents) {
  return new NativeBrowserViewMac(inspectable_web_contents);
}

}  // namespace electron
