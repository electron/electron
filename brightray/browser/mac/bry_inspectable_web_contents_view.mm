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
}
@end

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
  [_private->splitView release];
  [_private release];
  _private = nil;

  [super dealloc];
}

- (IBAction)showDevTools:(id)sender {
  _private->inspectableWebContentsView->inspectable_web_contents()->ShowDevTools();
}

- (void)setDevToolsVisible:(BOOL)visible {
  BOOL wasVisible = _private->splitView.subviews.count == 2;
  if (wasVisible == visible)
    return;

  auto devToolsWebContents = _private->inspectableWebContentsView->inspectable_web_contents()->devtools_web_contents();
  auto devToolsView = devToolsWebContents->GetView()->GetNativeView();

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

@end

@implementation BRYInspectableWebContentsViewPrivate
@end