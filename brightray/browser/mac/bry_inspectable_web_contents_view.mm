#import "browser/mac/bry_inspectable_web_contents_view.h"

#import "browser/inspectable_web_contents_impl.h"
#import "browser/inspectable_web_contents_view_mac.h"
#import "browser/mac/bry_inspectable_web_contents_view_private.h"

#import "content/public/browser/web_contents_view.h"

using namespace brightray;

@interface BRYInspectableWebContentsViewPrivate : NSObject {
@public
  InspectableWebContentsViewMac *inspectableWebContentsView;
  NSSplitView *splitView;
  NSWindow *window;
  BOOL visible;
}
@end

namespace {

NSRect devtoolsWindowFrame(NSView *referenceView) {
  auto screenFrame = [referenceView.window convertRectToScreen:[referenceView convertRect:referenceView.bounds toView:nil]];
  return NSInsetRect(screenFrame, NSWidth(screenFrame) / 6, NSHeight(screenFrame) / 6);
}

}

@implementation BRYInspectableWebContentsView

- (instancetype)initWithInspectableWebContentsViewMac:(InspectableWebContentsViewMac *)inspectableWebContentsView {
  self = [super init];
  if (!self)
    return nil;

  _private = [[BRYInspectableWebContentsViewPrivate alloc] init];
  _private->inspectableWebContentsView = inspectableWebContentsView;
  _private->splitView = [[NSSplitView alloc] init];

  [self addSubview:_private->splitView];
  _private->splitView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  _private->splitView.dividerStyle = NSSplitViewDividerStyleThin;
  [_private->splitView addSubview:inspectableWebContentsView->inspectable_web_contents()->GetWebContents()->GetView()->GetNativeView()];

  return self;
}

- (void)dealloc {
  [_private->window release];
  [_private->splitView release];
  [_private release];
  _private = nil;

  [super dealloc];
}

- (IBAction)showDevTools:(id)sender {
  _private->inspectableWebContentsView->inspectable_web_contents()->ShowDevTools();
}

- (void)setDevToolsVisible:(BOOL)visible {
  if (_private->visible == visible)
    return;
  _private->visible = visible;

  auto devToolsWebContents = _private->inspectableWebContentsView->inspectable_web_contents()->devtools_web_contents();
  auto devToolsView = devToolsWebContents->GetView()->GetNativeView();

  if (_private->window && devToolsView.window == _private->window) {
    if (visible) {
      [_private->window makeKeyAndOrderFront:nil];
    } else {
      [_private->window orderOut:nil];
    }
    return;
  }

  if (visible) {
    auto inspectedView = _private->inspectableWebContentsView->inspectable_web_contents()->GetWebContents()->GetView()->GetNativeView();
    CGRect frame = NSRectToCGRect(inspectedView.frame);
    CGRect inspectedViewFrame;
    CGRect devToolsFrame;
    CGRectDivide(frame, &inspectedViewFrame, &devToolsFrame, CGRectGetHeight(frame) * 2 / 3, CGRectMaxYEdge);

    inspectedView.frame = NSRectFromCGRect(inspectedViewFrame);
    devToolsView.frame = NSRectFromCGRect(devToolsFrame);

    [_private->splitView addSubview:devToolsView];
  } else {
    [devToolsView removeFromSuperview];
  }

  [_private->splitView adjustSubviews];
}

- (BOOL)setDockSide:(const std::string&)side {
  if (side == "right") {
    _private->splitView.vertical = YES;
    [self moveToSplitView];
  } else if (side == "bottom") {
    _private->splitView.vertical = NO;
    [self moveToSplitView];
  } else if (side == "undocked") {
    [self moveToWindow];
  } else {
    return NO;
  }

  return YES;
}

- (void)moveToWindow {
  if (!_private->window) {
    auto styleMask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask | NSTexturedBackgroundWindowMask | NSUnifiedTitleAndToolbarWindowMask;
    auto contentRect = [NSWindow contentRectForFrameRect:devtoolsWindowFrame(_private->splitView) styleMask:styleMask];
    _private->window = [[NSWindow alloc] initWithContentRect:contentRect styleMask:styleMask backing:NSBackingStoreBuffered defer:YES];
    _private->window.delegate = self;
    _private->window.releasedWhenClosed = NO;
    _private->window.title = @"Developer Tools";
    [_private->window setAutorecalculatesContentBorderThickness:NO forEdge:NSMaxYEdge];
    [_private->window setContentBorderThickness:24 forEdge:NSMaxYEdge];
  }

  auto devToolsWebContents = _private->inspectableWebContentsView->inspectable_web_contents()->devtools_web_contents();
  auto devToolsView = devToolsWebContents->GetView()->GetNativeView();

  NSView *contentView = _private->window.contentView;
  devToolsView.frame = contentView.bounds;
  devToolsView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

  [contentView addSubview:devToolsView];
  [_private->window makeKeyAndOrderFront:nil];
  [_private->splitView adjustSubviews];
}

- (void)moveToSplitView {
  [_private->window orderOut:nil];

  auto devToolsWebContents = _private->inspectableWebContentsView->inspectable_web_contents()->devtools_web_contents();
  auto devToolsView = devToolsWebContents->GetView()->GetNativeView();

  [_private->splitView addSubview:devToolsView];
  [_private->splitView adjustSubviews];
}

#pragma mark - NSWindowDelegate

- (BOOL)windowShouldClose:(id)sender {
  [_private->window orderOut:nil];
  return NO;
}

@end

@implementation BRYInspectableWebContentsViewPrivate
@end