#include "brightray/browser/mac/bry_inspectable_web_contents_view.h"

#include "brightray/browser/inspectable_web_contents_impl.h"
#include "brightray/browser/inspectable_web_contents_view_delegate.h"
#include "brightray/browser/inspectable_web_contents_view_mac.h"
#include "brightray/browser/mac/event_dispatching_window.h"
#include "content/public/browser/render_widget_host_view.h"
#include "ui/gfx/mac/scoped_cocoa_disable_screen_updates.h"

@implementation BRYInspectableWebContentsView

- (instancetype)initWithInspectableWebContentsViewMac:
    (InspectableWebContentsViewMac*)view {
  self = [super init];
  if (!self)
    return nil;

  inspectableWebContentsView_ = view;
  devtools_visible_ = NO;
  devtools_docked_ = NO;
  devtools_is_first_responder_ = NO;

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(viewDidBecomeFirstResponder:)
             name:kViewDidBecomeFirstResponder
           object:nil];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(parentWindowBecameMain:)
             name:NSWindowDidBecomeMainNotification
           object:nil];

  if (inspectableWebContentsView_->inspectable_web_contents()->IsGuest()) {
    fake_view_.reset([[NSView alloc] init]);
    [fake_view_ setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [self addSubview:fake_view_];
  } else {
    auto* contents = inspectableWebContentsView_->inspectable_web_contents()
                         ->GetWebContents();
    auto contentsView = contents->GetNativeView();
    [contentsView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [self addSubview:contentsView];
  }

  // See https://code.google.com/p/chromium/issues/detail?id=348490.
  [self setWantsLayer:YES];

  return self;
}

- (void)removeObservers {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize {
  [self adjustSubviews];
}

- (IBAction)showDevTools:(id)sender {
  inspectableWebContentsView_->inspectable_web_contents()->ShowDevTools();
}

- (void)notifyDevToolsFocused {
  if (inspectableWebContentsView_->GetDelegate())
    inspectableWebContentsView_->GetDelegate()->DevToolsFocused();
}

- (void)setDevToolsVisible:(BOOL)visible {
  if (visible == devtools_visible_)
    return;

  auto* inspectable_web_contents =
      inspectableWebContentsView_->inspectable_web_contents();
  auto* webContents = inspectable_web_contents->GetWebContents();
  auto* devToolsWebContents =
      inspectable_web_contents->GetDevToolsWebContents();
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
      gfx::ScopedCocoaDisableScreenUpdates disabler;
      [devToolsView removeFromSuperview];
      [self adjustSubviews];
    }
  } else {
    if (visible) {
      [devtools_window_ makeKeyAndOrderFront:nil];
    } else {
      [devtools_window_ setDelegate:nil];
      [devtools_window_ close];
      devtools_window_.reset();
    }
  }
}

- (BOOL)isDevToolsVisible {
  return devtools_visible_;
}

- (BOOL)isDevToolsFocused {
  if (devtools_docked_) {
    return [[self window] isKeyWindow] && devtools_is_first_responder_;
  } else {
    return [devtools_window_ isKeyWindow];
  }
}

- (void)setIsDocked:(BOOL)docked {
  // Revert to no-devtools state.
  [self setDevToolsVisible:NO];

  // Switch to new state.
  devtools_docked_ = docked;
  if (!docked) {
    auto* inspectable_web_contents =
        inspectableWebContentsView_->inspectable_web_contents();
    auto* devToolsWebContents =
        inspectable_web_contents->GetDevToolsWebContents();
    auto devToolsView = devToolsWebContents->GetNativeView();

    auto styleMask = NSTitledWindowMask | NSClosableWindowMask |
                     NSMiniaturizableWindowMask | NSResizableWindowMask |
                     NSTexturedBackgroundWindowMask |
                     NSUnifiedTitleAndToolbarWindowMask;
    devtools_window_.reset([[EventDispatchingWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 800, 600)
                  styleMask:styleMask
                    backing:NSBackingStoreBuffered
                      defer:YES]);
    [devtools_window_ setDelegate:self];
    [devtools_window_ setFrameAutosaveName:@"brightray.devtools"];
    [devtools_window_ setTitle:@"Developer Tools"];
    [devtools_window_ setReleasedWhenClosed:NO];
    [devtools_window_ setAutorecalculatesContentBorderThickness:NO
                                                        forEdge:NSMaxYEdge];
    [devtools_window_ setContentBorderThickness:24 forEdge:NSMaxYEdge];

    NSView* contentView = [devtools_window_ contentView];
    devToolsView.frame = contentView.bounds;
    devToolsView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    [contentView addSubview:devToolsView];
  }
  [self setDevToolsVisible:YES];
}

- (void)setContentsResizingStrategy:
    (const DevToolsContentsResizingStrategy&)strategy {
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

- (void)viewDidBecomeFirstResponder:(NSNotification*)notification {
  auto* inspectable_web_contents =
      inspectableWebContentsView_->inspectable_web_contents();
  if (!inspectable_web_contents || inspectable_web_contents->IsGuest())
    return;
  auto* webContents = inspectable_web_contents->GetWebContents();
  auto webContentsView = webContents->GetNativeView();

  NSView* view = [notification object];
  if ([[webContentsView subviews] containsObject:view]) {
    devtools_is_first_responder_ = NO;
    return;
  }

  auto* devToolsWebContents =
      inspectable_web_contents->GetDevToolsWebContents();
  if (!devToolsWebContents)
    return;
  auto devToolsView = devToolsWebContents->GetNativeView();

  if ([[devToolsView subviews] containsObject:view]) {
    devtools_is_first_responder_ = YES;
    [self notifyDevToolsFocused];
  }
}

- (void)parentWindowBecameMain:(NSNotification*)notification {
  NSWindow* parentWindow = [notification object];
  if ([self window] == parentWindow && devtools_docked_ &&
      devtools_is_first_responder_)
    [self notifyDevToolsFocused];
}

#pragma mark - NSWindowDelegate

- (void)windowWillClose:(NSNotification*)notification {
  inspectableWebContentsView_->inspectable_web_contents()->CloseDevTools();
}

- (void)windowDidBecomeMain:(NSNotification*)notification {
  content::WebContents* web_contents =
      inspectableWebContentsView_->inspectable_web_contents()
          ->GetDevToolsWebContents();
  if (!web_contents)
    return;

  web_contents->RestoreFocus();

  content::RenderWidgetHostView* rwhv = web_contents->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetActive(true);

  [self notifyDevToolsFocused];
}

- (void)windowDidResignMain:(NSNotification*)notification {
  content::WebContents* web_contents =
      inspectableWebContentsView_->inspectable_web_contents()
          ->GetDevToolsWebContents();
  if (!web_contents)
    return;

  web_contents->StoreFocus();

  content::RenderWidgetHostView* rwhv = web_contents->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetActive(false);
}

@end
