#import "browser/mac/bry_inspectable_web_contents_view.h"

#import "browser/inspectable_web_contents_impl.h"
#import "browser/inspectable_web_contents_view_mac.h"
#import "browser/mac/bry_inspectable_web_contents_view_private.h"

#import "content/public/browser/render_widget_host_view.h"
#import "content/public/browser/web_contents_view.h"
#import "ui/base/cocoa/underlay_opengl_hosting_window.h"

using namespace brightray;

@interface GraySplitView : NSSplitView
- (NSColor*)dividerColor;
@end

@implementation GraySplitView
- (NSColor*)dividerColor {
  return [NSColor darkGrayColor];
}
@end


@interface BRYInspectableWebContentsViewPrivate : NSObject {
@public
  InspectableWebContentsViewMac *inspectableWebContentsView;
  GraySplitView *splitView;
  NSWindow *window;
  BOOL visible;
}
@end

namespace {

NSRect devtoolsWindowFrame(NSView *referenceView) {
  auto screenFrame = [referenceView.window convertRectToScreen:[referenceView convertRect:referenceView.bounds toView:nil]];
  return NSInsetRect(screenFrame, NSWidth(screenFrame) / 6, NSHeight(screenFrame) / 6);
}

void SetActive(content::WebContents* web_contents, bool active) {
  auto render_widget_host_view = web_contents->GetRenderWidgetHostView();
  if (!render_widget_host_view)
    return;

  render_widget_host_view->SetActive(active);
}

}

@implementation BRYInspectableWebContentsView

- (instancetype)initWithInspectableWebContentsViewMac:(InspectableWebContentsViewMac *)inspectableWebContentsView {
  self = [super init];
  if (!self)
    return nil;

  _private = [[BRYInspectableWebContentsViewPrivate alloc] init];
  _private->inspectableWebContentsView = inspectableWebContentsView;
  _private->splitView = [[GraySplitView alloc] init];
  _private->splitView.delegate = self;

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

- (void)removeFromNotificationCenter {
  [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (IBAction)showDevTools:(id)sender {
  _private->inspectableWebContentsView->inspectable_web_contents()->ShowDevTools();
}

- (void)setDevToolsVisible:(BOOL)visible {
  if (_private->visible == visible)
    return;
  _private->visible = visible;

  if ([self isDocked]) {
    if (visible) {
      [_private->window makeKeyAndOrderFront:nil];
    } else {
      [_private->window orderOut:nil];
    }
    return;
  }

  auto devToolsWebContents = _private->inspectableWebContentsView->inspectable_web_contents()->devtools_web_contents();
  auto devToolsView = devToolsWebContents->GetView()->GetNativeView();

  if (visible) {
    auto inspectedView = _private->inspectableWebContentsView->inspectable_web_contents()->GetWebContents()->GetView()->GetNativeView();
    CGRect frame = NSRectToCGRect(inspectedView.frame);
    CGRect inspectedViewFrame;
    CGRect devToolsFrame;
    CGFloat amount;
    CGRectEdge edge;
    if ([_private->splitView isVertical]) {
      amount = CGRectGetWidth(frame) * 2 / 3;
      edge = CGRectMaxXEdge;
    } else {
      amount = CGRectGetHeight(frame) * 2 / 3;
      edge = CGRectMaxYEdge;
    }
    CGRectDivide(frame, &inspectedViewFrame, &devToolsFrame, amount, edge);

    inspectedView.frame = NSRectFromCGRect(inspectedViewFrame);
    devToolsView.frame = NSRectFromCGRect(devToolsFrame);

    [_private->splitView addSubview:devToolsView];
  } else {
    [devToolsView removeFromSuperview];
  }

  [_private->splitView adjustSubviews];
}

- (BOOL)isDevToolsVisible {
  return _private->visible;
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
    auto contentRect = [UnderlayOpenGLHostingWindow contentRectForFrameRect:devtoolsWindowFrame(_private->splitView) styleMask:styleMask];
    _private->window = [[UnderlayOpenGLHostingWindow alloc] initWithContentRect:contentRect styleMask:styleMask backing:NSBackingStoreBuffered defer:YES];
    _private->window.delegate = self;
    _private->window.releasedWhenClosed = NO;
    _private->window.title = @"Developer Tools";
    _private->window.frameAutosaveName = @"brightray.developer.tools";
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

- (BOOL)isDocked {
  auto devToolsWebContents = _private->inspectableWebContentsView->inspectable_web_contents()->devtools_web_contents();
  if (!devToolsWebContents)
    return NO;
  auto devToolsView = devToolsWebContents->GetView()->GetNativeView();

  return _private->window && devToolsView.window == _private->window;
}

- (void)window:(NSWindow *)window didBecomeActive:(BOOL)active {
  auto inspectable_contents = _private->inspectableWebContentsView->inspectable_web_contents();

  // Changes to the active state of the window we create only affects the dev tools contents.
  if (window == _private->window) {
    SetActive(inspectable_contents->devtools_web_contents(), active);
    return;
  }

  // Changes the window that hosts us always affect our main web contents. If the dev tools are also
  // hosted in this window, they are affected too.
  SetActive(inspectable_contents->GetWebContents(), active);
  if (![self isDocked])
    return;
  SetActive(inspectable_contents->devtools_web_contents(), active);
}

#pragma mark - NSView

- (void)viewWillMoveToWindow:(NSWindow *)newWindow {
  if (self.window) {
    [NSNotificationCenter.defaultCenter removeObserver:self name:NSWindowDidBecomeKeyNotification object:self.window];
    [NSNotificationCenter.defaultCenter removeObserver:self name:NSWindowDidResignKeyNotification object:self.window];
  }

  if (!newWindow)
    return;

  [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(windowDidBecomeKey:) name:NSWindowDidBecomeKeyNotification object:newWindow];
  [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(windowDidResignKey:) name:NSWindowDidResignKeyNotification object:newWindow];
}

#pragma mark - NSSplitViewDelegate

-(void)splitViewWillResizeSubviews:(NSNotification *)notification {
  [[_private->splitView window] disableScreenUpdatesUntilFlush];
}

#pragma mark - NSWindowDelegate

- (BOOL)windowShouldClose:(id)sender {
  _private->visible = NO;
  [_private->window orderOut:nil];
  return NO;
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
  [self window:notification.object didBecomeActive:YES];
}

- (void)windowDidResignKey:(NSNotification *)notification {
  [self window:notification.object didBecomeActive:NO];
}

@end

@implementation BRYInspectableWebContentsViewPrivate
@end
