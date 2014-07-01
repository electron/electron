#import "browser/mac/bry_inspectable_web_contents_view.h"

#import "browser/inspectable_web_contents_impl.h"
#import "browser/inspectable_web_contents_view_mac.h"

#import "content/public/browser/render_widget_host_view.h"
#import "content/public/browser/web_contents_view.h"
#import "ui/base/cocoa/underlay_opengl_hosting_window.h"

using namespace brightray;

@implementation BRYInspectableWebContentsView

- (instancetype)initWithInspectableWebContentsViewMac:(InspectableWebContentsViewMac*)inspectableWebContentsView {
  self = [super init];
  if (!self)
    return nil;

  inspectableWebContentsView_ = inspectableWebContentsView;
  devtools_visible_ = NO;

  auto webView = inspectableWebContentsView->inspectable_web_contents()->GetWebContents()->GetView()->GetNativeView();
  webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  [self addSubview:webView];

  return self;
}

- (void)dealloc {
  [super dealloc];
}

- (IBAction)showDevTools:(id)sender {
  inspectableWebContentsView_->inspectable_web_contents()->ShowDevTools();
}

- (void)setDevToolsVisible:(BOOL)visible {
  if (devtools_visible_ == visible)
    return;
  devtools_visible_ = visible;

  if (!devtools_window_) {
    auto devToolsWebContents = inspectableWebContentsView_->inspectable_web_contents()->devtools_web_contents();
    auto devToolsView = devToolsWebContents->GetView()->GetNativeView();

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
    [devtools_window_ setFrameAutosaveName:@"brightray.developer.tools"];
    [devtools_window_ setTitle:@"Developer Tools"];
    [devtools_window_ setReleasedWhenClosed:NO];
    [devtools_window_ setAutorecalculatesContentBorderThickness:NO forEdge:NSMaxYEdge];
    [devtools_window_ setContentBorderThickness:24 forEdge:NSMaxYEdge];

    NSView* contentView = [devtools_window_ contentView];
    devToolsView.frame = contentView.bounds;
    devToolsView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    [contentView addSubview:devToolsView];
  }

  if (visible) {
    [devtools_window_ makeKeyAndOrderFront:nil];
  } else {
    [devtools_window_ performClose:nil];
  }
}

- (BOOL)isDevToolsVisible {
  return devtools_visible_;
}

- (BOOL)setDockSide:(const std::string&)side {
  return NO;
}

#pragma mark - NSWindowDelegate

- (void)windowWillClose:(NSNotification*)notification {
  devtools_visible_ = NO;
  devtools_window_.reset();
}

@end
