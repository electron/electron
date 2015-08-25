#include "browser/mac/bry_inspectable_web_contents_view.h"

#import <QuartzCore/QuartzCore.h>

#include "browser/inspectable_web_contents_impl.h"
#include "browser/inspectable_web_contents_view_mac.h"

#include "content/public/browser/render_widget_host_view.h"
#include "ui/base/cocoa/animation_utils.h"
#include "ui/base/cocoa/underlay_opengl_hosting_window.h"
#include "ui/gfx/mac/scoped_ns_disable_screen_updates.h"

using namespace brightray;

namespace {

// The radius of the rounded corners.
const CGFloat kRoundedCornerRadius = 4;

}  // namespace

@implementation BRYInspectableWebContentsView

- (instancetype)initWithInspectableWebContentsViewMac:(InspectableWebContentsViewMac*)view {
  self = [super init];
  if (!self)
    return nil;

  inspectableWebContentsView_ = view;
  devtools_visible_ = NO;
  devtools_docked_ = NO;

  auto contents = inspectableWebContentsView_->inspectable_web_contents()->GetWebContents();
  auto contentsView = contents->GetNativeView();
  [contentsView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  [self addSubview:contentsView];

  // See https://code.google.com/p/chromium/issues/detail?id=348490.
  ScopedCAActionDisabler disabler;
  base::scoped_nsobject<CALayer> layer([[CALayer alloc] init]);
  [self setLayer:layer];
  [self setWantsLayer:YES];

  // See https://code.google.com/p/chromium/issues/detail?id=396264.
  [self updateLayerMask];

  return self;
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize {
  [self adjustSubviews];
}

// Every time the frame's size changes, the layer mask needs to be updated.
- (void)setFrameSize:(NSSize)newSize {
  [super setFrameSize:newSize];
  [self updateLayerMask];
}

- (IBAction)showDevTools:(id)sender {
  inspectableWebContentsView_->inspectable_web_contents()->ShowDevTools();
}

- (void)setDevToolsVisible:(BOOL)visible {
  if (visible == devtools_visible_)
    return;

  auto inspectable_web_contents = inspectableWebContentsView_->inspectable_web_contents();
  auto webContents = inspectable_web_contents->GetWebContents();
  auto devToolsWebContents = inspectable_web_contents->GetDevToolsWebContents();
  auto devToolsView = devToolsWebContents->GetNativeView();

  if (visible && devtools_docked_) {
    webContents->SetAllowOtherViews(true);
    devToolsWebContents->SetAllowOtherViews(true);
  } else {
    webContents->SetAllowOtherViews(false);
  }

  devtools_visible_ = visible;
  if (devtools_docked_) {
    if (visible) {
      // Place the devToolsView under contentsView, notice that we didn't set
      // sizes for them until the setContentsResizingStrategy message.
      [self addSubview:devToolsView positioned:NSWindowBelow relativeTo:nil];
      [self adjustSubviews];

      // Focus on web view.
      devToolsWebContents->RestoreFocus();
    } else {
      gfx::ScopedNSDisableScreenUpdates disabler;
      [devToolsView removeFromSuperview];
      [self adjustSubviews];
    }
  } else {
    if (visible) {
      [devtools_window_ makeKeyAndOrderFront:nil];
    } else {
      [[self window] makeKeyAndOrderFront:nil];
      [devtools_window_ setDelegate:nil];
      [devtools_window_ close];
      devtools_window_.reset();
    }
  }
}

- (BOOL)isDevToolsVisible {
  return devtools_visible_;
}

- (void)setIsDocked:(BOOL)docked {
  // Revert to no-devtools state.
  [self setDevToolsVisible:NO];

  // Switch to new state.
  devtools_docked_ = docked;
  if (!docked) {
    auto inspectable_web_contents = inspectableWebContentsView_->inspectable_web_contents();
    auto devToolsWebContents = inspectable_web_contents->GetDevToolsWebContents();
    auto devToolsView = devToolsWebContents->GetNativeView();

    auto styleMask = NSTitledWindowMask | NSClosableWindowMask |
                     NSMiniaturizableWindowMask | NSResizableWindowMask |
                     NSTexturedBackgroundWindowMask |
                     NSUnifiedTitleAndToolbarWindowMask;
    devtools_window_.reset([[UnderlayOpenGLHostingWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 800, 600)
                  styleMask:styleMask
                    backing:NSBackingStoreBuffered
                      defer:YES]);
    [devtools_window_ setDelegate:self];
    [devtools_window_ setFrameAutosaveName:@"brightray.devtools"];
    [devtools_window_ setTitle:@"Developer Tools"];
    [devtools_window_ setReleasedWhenClosed:NO];
    [devtools_window_ setAutorecalculatesContentBorderThickness:NO forEdge:NSMaxYEdge];
    [devtools_window_ setContentBorderThickness:24 forEdge:NSMaxYEdge];

    NSView* contentView = [devtools_window_ contentView];
    devToolsView.frame = contentView.bounds;
    devToolsView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    [contentView addSubview:devToolsView];
  }
  [self setDevToolsVisible:YES];
}

- (void)setContentsResizingStrategy:(const DevToolsContentsResizingStrategy&)strategy {
  strategy_.CopyFrom(strategy);
  [self adjustSubviews];
}

- (void)adjustSubviews {
  if (![[self subviews] count])
    return;

  if (![self isDevToolsVisible] || devtools_window_) {
    DCHECK_EQ(1u, [[self subviews] count]);
    NSView* contents = [[self subviews] objectAtIndex:0];
    [contents setFrame:[self bounds]];
    return;
  }

  NSView* devToolsView = [[self subviews] objectAtIndex:0];
  NSView* contentsView = [[self subviews] objectAtIndex:1];

  DCHECK_EQ(2u, [[self subviews] count]);

  gfx::Rect new_devtools_bounds;
  gfx::Rect new_contents_bounds;
  ApplyDevToolsContentsResizingStrategy(
      strategy_, gfx::Size(NSSizeToCGSize([self bounds].size)),
      &new_devtools_bounds, &new_contents_bounds);
  [devToolsView setFrame:[self flipRectToNSRect:new_devtools_bounds]];
  [contentsView setFrame:[self flipRectToNSRect:new_contents_bounds]];
}

- (void)setTitle:(NSString*)title {
  [devtools_window_ setTitle:title];
}

// Creates a path whose bottom two corners are rounded.
// Caller takes ownership of the path.
- (CGPathRef)createRoundedBottomCornersPath:(NSSize)size {
  CGMutablePathRef path = CGPathCreateMutable();
  CGFloat width = size.width;
  CGFloat height = size.height;
  CGFloat cornerRadius = kRoundedCornerRadius;

  // Top left corner.
  CGPathMoveToPoint(path, NULL, 0, height);

  // Top right corner.
  CGPathAddLineToPoint(path, NULL, width, height);

  // Bottom right corner.
  CGPathAddArc(path,
               NULL,
               width - cornerRadius,
               cornerRadius,
               cornerRadius,
               0,
               -M_PI_2,
               true);

  // Bottom left corner.
  CGPathAddArc(path,
               NULL,
               cornerRadius,
               cornerRadius,
               cornerRadius,
               -M_PI_2,
               -M_PI,
               true);

  // Draw line back to top-left corner.
  CGPathAddLineToPoint(path, NULL, 0, height);
  CGPathCloseSubpath(path);
  return path;
}

// Updates the path of the layer mask to have make window have round corner.
- (void)updateLayerMask {
  if (![self layer].mask) {
    layerMask_ = [CAShapeLayer layer];
    [self layer].mask = layerMask_;
  }

  CGPathRef path = [self createRoundedBottomCornersPath:self.bounds.size];
  layerMask_.path = path;
  CGPathRelease(path);
}

#pragma mark - NSWindowDelegate

- (void)windowWillClose:(NSNotification*)notification {
  inspectableWebContentsView_->inspectable_web_contents()->CloseDevTools();
}

- (void)windowDidBecomeMain:(NSNotification*)notification {
  content::WebContents* web_contents =
      inspectableWebContentsView_->inspectable_web_contents()->GetDevToolsWebContents();
  if (!web_contents)
    return;

  web_contents->RestoreFocus();

  content::RenderWidgetHostView* rwhv = web_contents->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetActive(true);
}

- (void)windowDidResignMain:(NSNotification*)notification {
  content::WebContents* web_contents =
      inspectableWebContentsView_->inspectable_web_contents()->GetDevToolsWebContents();
  if (!web_contents)
    return;

  web_contents->StoreFocus();

  content::RenderWidgetHostView* rwhv = web_contents->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetActive(false);
}

@end
