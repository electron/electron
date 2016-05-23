// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window_mac.h"

#include <string>

#include "atom/common/color_util.h"
#include "atom/common/draggable_region.h"
#include "atom/common/options_switches.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/strings/sys_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "native_mate/dictionary.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/skia_util.h"

namespace {

// Prevents window from resizing during the scope.
class ScopedDisableResize {
 public:
  ScopedDisableResize() { disable_resize_ = true; }
  ~ScopedDisableResize() { disable_resize_ = false; }

  static bool IsResizeDisabled() { return disable_resize_; }

 private:
  static bool disable_resize_;
};

bool ScopedDisableResize::disable_resize_ = false;

}  // namespace

// This view always takes the size of its superview. It is intended to be used
// as a NSWindow's contentView.  It is needed because NSWindow's implementation
// explicitly resizes the contentView at inopportune times.
@interface FullSizeContentView : NSView
@end

@implementation FullSizeContentView

// This method is directly called by NSWindow during a window resize on OSX
// 10.10.0, beta 2. We must override it to prevent the content view from
// shrinking.
- (void)setFrameSize:(NSSize)size {
  if ([self superview])
    size = [[self superview] bounds].size;
  [super setFrameSize:size];
}

// The contentView gets moved around during certain full-screen operations.
// This is less than ideal, and should eventually be removed.
- (void)viewDidMoveToSuperview {
  [self setFrame:[[self superview] bounds]];
}

@end

@interface AtomNSWindowDelegate : NSObject<NSWindowDelegate> {
 @private
  atom::NativeWindowMac* shell_;
}
- (id)initWithShell:(atom::NativeWindowMac*)shell;
@end

@implementation AtomNSWindowDelegate

- (id)initWithShell:(atom::NativeWindowMac*)shell {
  if ((self = [super init])) {
    shell_ = shell;
  }
  return self;
}

- (void)windowDidChangeOcclusionState:(NSNotification *)notification {
  // notification.object is the window that changed its state.
  // It's safe to use self.window instead if you don't assign one delegate to many windows
  NSWindow *window = notification.object;

  // check occlusion binary flag
  if (window.occlusionState & NSWindowOcclusionStateVisible)   {
     // The app is visible
     shell_->NotifyWindowShow();
   } else {
     // The app is not visible
     shell_->NotifyWindowHide();
   }
}

- (void)windowDidBecomeMain:(NSNotification*)notification {
  content::WebContents* web_contents = shell_->web_contents();
  if (!web_contents)
    return;

  web_contents->RestoreFocus();

  content::RenderWidgetHostView* rwhv = web_contents->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetActive(true);

  shell_->NotifyWindowFocus();
}

- (void)windowDidResignMain:(NSNotification*)notification {
  content::WebContents* web_contents = shell_->web_contents();
  if (!web_contents)
    return;

  web_contents->StoreFocus();

  content::RenderWidgetHostView* rwhv = web_contents->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetActive(false);

  shell_->NotifyWindowBlur();
}

- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize {
  NSSize newSize = frameSize;
  double aspectRatio = shell_->GetAspectRatio();

  if (aspectRatio > 0.0) {
    gfx::Size windowSize = shell_->GetSize();
    gfx::Size contentSize = shell_->GetContentSize();
    gfx::Size extraSize = shell_->GetAspectRatioExtraSize();

    double extraWidthPlusFrame =
        windowSize.width() - contentSize.width() + extraSize.width();
    double extraHeightPlusFrame =
        windowSize.height() - contentSize.height() + extraSize.height();

    newSize.width =
        roundf((frameSize.height - extraHeightPlusFrame) * aspectRatio +
               extraWidthPlusFrame);
    newSize.height =
        roundf((newSize.width - extraWidthPlusFrame) / aspectRatio +
               extraHeightPlusFrame);
  }

  return newSize;
}

- (void)windowDidResize:(NSNotification*)notification {
  shell_->UpdateDraggableRegionViews();
  shell_->NotifyWindowResize();
}

- (void)windowDidMove:(NSNotification*)notification {
  // TODO(zcbenz): Remove the alias after figuring out a proper
  // way to disptach move.
  shell_->NotifyWindowMove();
  shell_->NotifyWindowMoved();
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
  shell_->NotifyWindowMinimize();
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
  shell_->NotifyWindowRestore();
}

- (BOOL)windowShouldZoom:(NSWindow*)window toFrame:(NSRect)newFrame {
  // Cocoa doen't have concept of maximize/unmaximize, so wee need to emulate
  // them by calculating size change when zooming.
  if (newFrame.size.width < [window frame].size.width ||
      newFrame.size.height < [window frame].size.height)
    shell_->NotifyWindowUnmaximize();
  else
    shell_->NotifyWindowMaximize();
  return YES;
}

- (void)windowWillEnterFullScreen:(NSNotification*)notification {
  // Hide the native toolbar before entering fullscreen, so there is no visual
  // artifacts.
  if (shell_->should_hide_native_toolbar_in_fullscreen()) {
    NSWindow* window = shell_->GetNativeWindow();
    [window setToolbar:nil];
  }
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
  shell_->NotifyWindowEnterFullScreen();

  // Restore the native toolbar immediately after entering fullscreen, if we do
  // this before leaving fullscreen, traffic light buttons will be jumping.
  if (shell_->should_hide_native_toolbar_in_fullscreen()) {
    NSWindow* window = shell_->GetNativeWindow();
    base::scoped_nsobject<NSToolbar> toolbar(
        [[NSToolbar alloc] initWithIdentifier:@"titlebarStylingToolbar"]);
    [toolbar setShowsBaselineSeparator:NO];
    [window setToolbar:toolbar];

    // Set window style to hide the toolbar, otherwise the toolbar will show in
    // fullscreen mode.
    shell_->SetStyleMask(true, NSFullSizeContentViewWindowMask);
  }
}

- (void)windowWillExitFullScreen:(NSNotification*)notification {
  // Turn off the style for toolbar.
  if (shell_->should_hide_native_toolbar_in_fullscreen())
    shell_->SetStyleMask(false, NSFullSizeContentViewWindowMask);
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
  // For certain versions of OS X the fullscreen button will automatically show
  // after exiting fullscreen mode.
  if (!shell_->has_frame()) {
    NSWindow* window = shell_->GetNativeWindow();
    [[window standardWindowButton:NSWindowFullScreenButton] setHidden:YES];
  }

  shell_->NotifyWindowLeaveFullScreen();
}

- (void)windowWillClose:(NSNotification*)notification {
  shell_->NotifyWindowClosed();

  // Clears the delegate when window is going to be closed, since EL Capitan it
  // is possible that the methods of delegate would get called after the window
  // has been closed.
  [shell_->GetNativeWindow() setDelegate:nil];
}

- (BOOL)windowShouldClose:(id)window {
  // When user tries to close the window by clicking the close button, we do
  // not close the window immediately, instead we try to close the web page
  // fisrt, and when the web page is closed the window will also be closed.
  shell_->RequestToClosePage();
  return NO;
}

- (NSRect)window:(NSWindow*)window
    willPositionSheet:(NSWindow*)sheet usingRect:(NSRect)rect {
  NSView* view = window.contentView;
  rect.origin.y = view.frame.size.height - shell_->GetSheetOffset();
  return rect;
}

@end

@interface AtomNSWindow : NSWindow {
 @private
  atom::NativeWindowMac* shell_;
  bool enable_larger_than_screen_;
}
@property BOOL acceptsFirstMouse;
@property BOOL disableAutoHideCursor;
@property BOOL disableKeyOrMainWindow;

- (void)setShell:(atom::NativeWindowMac*)shell;
- (void)setEnableLargerThanScreen:(bool)enable;
@end

@implementation AtomNSWindow

- (void)setShell:(atom::NativeWindowMac*)shell {
  shell_ = shell;
}

- (void)setEnableLargerThanScreen:(bool)enable {
  enable_larger_than_screen_ = enable;
}

// NSWindow overrides.

- (void)swipeWithEvent:(NSEvent *)event {
  if (event.deltaY == 1.0) {
    shell_->NotifyWindowSwipe("up");
  } else if (event.deltaX == -1.0) {
    shell_->NotifyWindowSwipe("right");
  } else if (event.deltaY == -1.0) {
    shell_->NotifyWindowSwipe("down");
  } else if (event.deltaX == 1.0) {
    shell_->NotifyWindowSwipe("left");
  }
}

- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen*)screen {
  // Resizing is disabled.
  if (ScopedDisableResize::IsResizeDisabled())
    return [self frame];

  // Enable the window to be larger than screen.
  if (enable_larger_than_screen_)
    return frameRect;
  else
    return [super constrainFrameRect:frameRect toScreen:screen];
}

- (id)accessibilityAttributeValue:(NSString*)attribute {
  if (![attribute isEqualToString:@"AXChildren"])
    return [super accessibilityAttributeValue:attribute];

  // Filter out objects that aren't the title bar buttons. This has the effect
  // of removing the window title, which VoiceOver already sees.
  // * when VoiceOver is disabled, this causes Cmd+C to be used for TTS but
  //   still leaves the buttons available in the accessibility tree.
  // * when VoiceOver is enabled, the full accessibility tree is used.
  // Without removing the title and with VO disabled, the TTS would always read
  // the window title instead of using Cmd+C to get the selected text.
  NSPredicate *predicate = [NSPredicate predicateWithFormat:
      @"(self isKindOfClass: %@) OR (self.className == %@)",
      [NSButtonCell class],
      @"RenderWidgetHostViewCocoa"];

  NSArray *children = [super accessibilityAttributeValue:attribute];
  return [children filteredArrayUsingPredicate:predicate];
}

- (BOOL)canBecomeMainWindow {
  return !self.disableKeyOrMainWindow;
}

- (BOOL)canBecomeKeyWindow {
  return !self.disableKeyOrMainWindow;
}

@end

@interface ControlRegionView : NSView
@end

@implementation ControlRegionView

- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

- (NSView*)hitTest:(NSPoint)aPoint {
  return nil;
}

@end

@interface NSView (WebContentsView)
- (void)setMouseDownCanMoveWindow:(BOOL)can_move;
@end

@interface AtomProgressBar : NSProgressIndicator
@end

@implementation AtomProgressBar

- (void)drawRect:(NSRect)dirtyRect {
  if (self.style != NSProgressIndicatorBarStyle)
    return;
  // Draw edges of rounded rect.
  NSRect rect = NSInsetRect([self bounds], 1.0, 1.0);
  CGFloat radius = rect.size.height / 2;
  NSBezierPath* bezier_path = [NSBezierPath bezierPathWithRoundedRect:rect xRadius:radius yRadius:radius];
  [bezier_path setLineWidth:2.0];
  [[NSColor grayColor] set];
  [bezier_path stroke];

  // Fill the rounded rect.
  rect = NSInsetRect(rect, 2.0, 2.0);
  radius = rect.size.height / 2;
  bezier_path = [NSBezierPath bezierPathWithRoundedRect:rect xRadius:radius yRadius:radius];
  [bezier_path setLineWidth:1.0];
  [bezier_path addClip];

  // Calculate the progress width.
  rect.size.width = floor(rect.size.width * ([self doubleValue] / [self maxValue]));

  // Fill the progress bar with color blue.
  [[NSColor colorWithSRGBRed:0.2 green:0.6 blue:1 alpha:1] set];
  NSRectFill(rect);
}

@end

namespace atom {

NativeWindowMac::NativeWindowMac(
    brightray::InspectableWebContents* web_contents,
    const mate::Dictionary& options)
    : NativeWindow(web_contents, options),
      is_kiosk_(false),
      attention_request_id_(0),
      force_show_buttons_(false),
      should_hide_native_toolbar_in_fullscreen_(false) {
  int width = 800, height = 600;
  options.Get(options::kWidth, &width);
  options.Get(options::kHeight, &height);

  NSRect main_screen_rect = [[[NSScreen screens] objectAtIndex:0] frame];
  NSRect cocoa_bounds = NSMakeRect(
      round((NSWidth(main_screen_rect) - width) / 2) ,
      round((NSHeight(main_screen_rect) - height) / 2),
      width,
      height);

  bool resizable = true;
  options.Get(options::kResizable, &resizable);

  bool minimizable = true;
  options.Get(options::kMinimizable, &minimizable);

  bool maximizable = true;
  options.Get(options::kMaximizable, &maximizable);

  bool closable = true;
  options.Get(options::kClosable, &closable);

  // New title bar styles are available in Yosemite or newer
  std::string titleBarStyle;
  if (base::mac::IsOSYosemiteOrLater())
    options.Get(options::kTitleBarStyle, &titleBarStyle);

  std::string windowType;
  options.Get(options::kType, &windowType);

  bool useStandardWindow = true;
  // eventually deprecate separate "standardWindow" option in favor of
  // standard / textured window types
  options.Get(options::kStandardWindow, &useStandardWindow);
  if (windowType == "textured") {
    useStandardWindow = false;
  }

  NSUInteger styleMask = NSTitledWindowMask;
  if (minimizable) {
    styleMask |= NSMiniaturizableWindowMask;
  }
  if (closable) {
    styleMask |= NSClosableWindowMask;
  }
  if ((titleBarStyle == "hidden") || (titleBarStyle == "hidden-inset")) {
    // The window without titlebar is treated the same with frameless window.
    set_has_frame(false);
    force_show_buttons_ = true;
  }
  if (!useStandardWindow || transparent() || !has_frame()) {
    styleMask |= NSTexturedBackgroundWindowMask;
  }
  if (resizable) {
    styleMask |= NSResizableWindowMask;
  }

  window_.reset([[AtomNSWindow alloc]
      initWithContentRect:cocoa_bounds
                styleMask:styleMask
                  backing:NSBackingStoreBuffered
                    defer:YES]);
  [window_ setShell:this];
  [window_ setEnableLargerThanScreen:enable_larger_than_screen()];

  window_delegate_.reset([[AtomNSWindowDelegate alloc] initWithShell:this]);
  [window_ setDelegate:window_delegate_];

  if (transparent()) {
    // Make window has transparent background.
    [window_ setOpaque:NO];
    // Setting the background color to clear will also hide the shadow.
    [window_ setBackgroundColor:[NSColor clearColor]];
  }

  if (windowType == "desktop") {
    [window_ setLevel:kCGDesktopWindowLevel - 1];
    [window_ setDisableKeyOrMainWindow:YES];
    [window_ setCollectionBehavior:
        (NSWindowCollectionBehaviorCanJoinAllSpaces |
         NSWindowCollectionBehaviorStationary |
         NSWindowCollectionBehaviorIgnoresCycle)];
  }

  // Remove non-transparent corners, see http://git.io/vfonD.
  if (!has_frame())
    [window_ setOpaque:NO];

  // We will manage window's lifetime ourselves.
  [window_ setReleasedWhenClosed:NO];

  // Hide the title bar.
  if (titleBarStyle == "hidden-inset") {
    [window_ setTitlebarAppearsTransparent:YES];
    [window_ setTitleVisibility:NSWindowTitleHidden];
    if (titleBarStyle == "hidden-inset") {
      base::scoped_nsobject<NSToolbar> toolbar(
          [[NSToolbar alloc] initWithIdentifier:@"titlebarStylingToolbar"]);
      [toolbar setShowsBaselineSeparator:NO];
      [window_ setToolbar:toolbar];
      should_hide_native_toolbar_in_fullscreen_ = true;
    }
  }

  // On OS X the initial window size doesn't include window frame.
  bool use_content_size = false;
  options.Get(options::kUseContentSize, &use_content_size);
  if (!has_frame() || !use_content_size)
    SetSize(gfx::Size(width, height));

  // Enable the NSView to accept first mouse event.
  bool acceptsFirstMouse = false;
  options.Get(options::kAcceptFirstMouse, &acceptsFirstMouse);
  [window_ setAcceptsFirstMouse:acceptsFirstMouse];

  // Disable auto-hiding cursor.
  bool disableAutoHideCursor = false;
  options.Get(options::kDisableAutoHideCursor, &disableAutoHideCursor);
  [window_ setDisableAutoHideCursor:disableAutoHideCursor];

  // Disable zoom button if window is not resizable.
  if (!maximizable)
    SetMaximizable(false);

  NSView* view = inspectable_web_contents()->GetView()->GetNativeView();
  [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

  // Use an NSEvent monitor to listen for the wheel event.
  BOOL __block began = NO;
  wheel_event_monitor_ = [NSEvent
    addLocalMonitorForEventsMatchingMask:NSScrollWheelMask
    handler:^(NSEvent* event) {
      if ([[event window] windowNumber] != [window_ windowNumber])
        return event;

      if (!web_contents)
        return event;

      if (!began && (([event phase] == NSEventPhaseMayBegin) ||
                     ([event phase] == NSEventPhaseBegan))) {
        this->NotifyWindowScrollTouchBegin();
        began = YES;
      } else if (began && (([event phase] == NSEventPhaseEnded) ||
                           ([event phase] == NSEventPhaseCancelled))) {
        this->NotifyWindowScrollTouchEnd();
        began = NO;
      }
      return event;
  }];

  InstallView();
}

NativeWindowMac::~NativeWindowMac() {
  [NSEvent removeMonitor:wheel_event_monitor_];
  Observe(nullptr);
}

void NativeWindowMac::Close() {
  [window_ performClose:nil];
}

void NativeWindowMac::CloseImmediately() {
  [window_ close];
}

void NativeWindowMac::Focus(bool focus) {
  if (!IsVisible())
    return;

  if (focus) {
    [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
    [window_ makeKeyAndOrderFront:nil];
  } else {
    [window_ orderBack:nil];
  }
}

bool NativeWindowMac::IsFocused() {
  return [window_ isKeyWindow];
}

void NativeWindowMac::Show() {
  // This method is supposed to put focus on window, however if the app does not
  // have focus then "makeKeyAndOrderFront" will only show the window.
  [NSApp activateIgnoringOtherApps:YES];

  [window_ makeKeyAndOrderFront:nil];
}

void NativeWindowMac::ShowInactive() {
  [window_ orderFrontRegardless];
}

void NativeWindowMac::Hide() {
  [window_ orderOut:nil];
}

bool NativeWindowMac::IsVisible() {
  return [window_ isVisible];
}

void NativeWindowMac::Maximize() {
  if (IsMaximized())
    return;

  [window_ zoom:nil];
}

void NativeWindowMac::Unmaximize() {
  if (!IsMaximized())
    return;

  [window_ zoom:nil];
}

bool NativeWindowMac::IsMaximized() {
  if (([window_ styleMask] & NSResizableWindowMask) != 0) {
    return [window_ isZoomed];
  } else {
    NSRect rectScreen = [[NSScreen mainScreen] visibleFrame];
    NSRect rectWindow = [window_ frame];
    return (rectScreen.origin.x == rectWindow.origin.x &&
            rectScreen.origin.y == rectWindow.origin.y &&
            rectScreen.size.width == rectWindow.size.width &&
            rectScreen.size.height == rectWindow.size.height);
  }
}

void NativeWindowMac::Minimize() {
  [window_ miniaturize:nil];
}

void NativeWindowMac::Restore() {
  [window_ deminiaturize:nil];
}

bool NativeWindowMac::IsMinimized() {
  return [window_ isMiniaturized];
}

void NativeWindowMac::SetFullScreen(bool fullscreen) {
  if (fullscreen == IsFullscreen())
    return;

  if (!base::mac::IsOSLionOrLater()) {
    LOG(ERROR) << "Fullscreen mode is only supported above Lion";
    return;
  }

  [window_ toggleFullScreen:nil];
}

bool NativeWindowMac::IsFullscreen() const {
  return [window_ styleMask] & NSFullScreenWindowMask;
}

void NativeWindowMac::SetBounds(const gfx::Rect& bounds, bool animate) {
  NSRect cocoa_bounds = NSMakeRect(bounds.x(), 0,
                                   bounds.width(),
                                   bounds.height());
  // Flip coordinates based on the primary screen.
  NSScreen* screen = [[NSScreen screens] objectAtIndex:0];
  cocoa_bounds.origin.y =
      NSHeight([screen frame]) - bounds.height() - bounds.y();

  [window_ setFrame:cocoa_bounds display:YES animate:animate];
}

gfx::Rect NativeWindowMac::GetBounds() {
  NSRect frame = [window_ frame];
  gfx::Rect bounds(frame.origin.x, 0, NSWidth(frame), NSHeight(frame));
  NSScreen* screen = [[NSScreen screens] objectAtIndex:0];
  bounds.set_y(NSHeight([screen frame]) - NSMaxY(frame));
  return bounds;
}

void NativeWindowMac::SetContentSizeConstraints(
    const extensions::SizeConstraints& size_constraints) {
  auto convertSize = [this](const gfx::Size& size) {
    // Our frameless window still has titlebar attached, so setting contentSize
    // will result in actual content size being larger.
    if (!has_frame()) {
      NSRect frame = NSMakeRect(0, 0, size.width(), size.height());
      NSRect content = [window_ contentRectForFrameRect:frame];
      return content.size;
    } else {
      return NSMakeSize(size.width(), size.height());
    }
  };

  NSView* content = [window_ contentView];
  if (size_constraints.HasMinimumSize()) {
    NSSize min_size = convertSize(size_constraints.GetMinimumSize());
    [window_ setContentMinSize:[content convertSize:min_size toView:nil]];
  }
  if (size_constraints.HasMaximumSize()) {
    NSSize max_size = convertSize(size_constraints.GetMaximumSize());
    [window_ setContentMaxSize:[content convertSize:max_size toView:nil]];
  }
  NativeWindow::SetContentSizeConstraints(size_constraints);
}

void NativeWindowMac::SetResizable(bool resizable) {
  // Change styleMask for frameless causes the window to change size, so we have
  // to explicitly disables that.
  ScopedDisableResize disable_resize;
  SetStyleMask(resizable, NSResizableWindowMask);
}

bool NativeWindowMac::IsResizable() {
  return [window_ styleMask] & NSResizableWindowMask;
}

void NativeWindowMac::SetAspectRatio(double aspect_ratio,
                                     const gfx::Size& extra_size) {
    NativeWindow::SetAspectRatio(aspect_ratio, extra_size);

    // We can't just pass the aspect ratio to Cocoa, since our API receives
    // it as a float, and Cocoa expects an NSRect with explicit width & height
    // arguments. Instead we derive those args ourselves from the given aspect
    // ratio.

    gfx::Size windowSize = this->GetSize();
    gfx::Size contentSize = this->GetContentSize();

    double extraWidthPlusFrame =
        windowSize.width() - contentSize.width() + extra_size.width();
    double extraHeightPlusFrame =
        windowSize.height() - contentSize.height() + extra_size.height();

    double width =
        roundf(([window_ frame].size.height - extraHeightPlusFrame) *
        aspect_ratio + extraWidthPlusFrame);
    double height =
        roundf((width - extraWidthPlusFrame) /
        aspect_ratio + extraHeightPlusFrame);

    [window_ setAspectRatio:NSMakeSize(width, height)];
}

void NativeWindowMac::SetMovable(bool movable) {
  [window_ setMovable:movable];
}

bool NativeWindowMac::IsMovable() {
  return [window_ isMovable];
}

void NativeWindowMac::SetMinimizable(bool minimizable) {
  SetStyleMask(minimizable, NSMiniaturizableWindowMask);
}

bool NativeWindowMac::IsMinimizable() {
  return [window_ styleMask] & NSMiniaturizableWindowMask;
}

void NativeWindowMac::SetMaximizable(bool maximizable) {
  [[window_ standardWindowButton:NSWindowZoomButton] setEnabled:maximizable];
}

bool NativeWindowMac::IsMaximizable() {
  return [[window_ standardWindowButton:NSWindowZoomButton] isEnabled];
}

void NativeWindowMac::SetFullScreenable(bool fullscreenable) {
  SetCollectionBehavior(
      fullscreenable, NSWindowCollectionBehaviorFullScreenPrimary);
  // On EL Capitan this flag is required to hide fullscreen button.
  SetCollectionBehavior(
      !fullscreenable, NSWindowCollectionBehaviorFullScreenAuxiliary);
}

bool NativeWindowMac::IsFullScreenable() {
  NSUInteger collectionBehavior = [window_ collectionBehavior];
  return collectionBehavior & NSWindowCollectionBehaviorFullScreenPrimary;
}

void NativeWindowMac::SetClosable(bool closable) {
  SetStyleMask(closable, NSClosableWindowMask);
}

bool NativeWindowMac::IsClosable() {
  return [window_ styleMask] & NSClosableWindowMask;
}

void NativeWindowMac::SetAlwaysOnTop(bool top) {
  [window_ setLevel:(top ? NSFloatingWindowLevel : NSNormalWindowLevel)];
}

bool NativeWindowMac::IsAlwaysOnTop() {
  return [window_ level] == NSFloatingWindowLevel;
}

void NativeWindowMac::Center() {
  [window_ center];
}

void NativeWindowMac::SetTitle(const std::string& title) {
  // We don't want the title to show in transparent or frameless window.
  if (transparent() || !has_frame())
    return;

  [window_ setTitle:base::SysUTF8ToNSString(title)];
}

std::string NativeWindowMac::GetTitle() {
  return base::SysNSStringToUTF8([window_ title]);
}

void NativeWindowMac::FlashFrame(bool flash) {
  if (flash) {
    attention_request_id_ = [NSApp requestUserAttention:NSInformationalRequest];
  } else {
    [NSApp cancelUserAttentionRequest:attention_request_id_];
    attention_request_id_ = 0;
  }
}

void NativeWindowMac::SetSkipTaskbar(bool skip) {
}

void NativeWindowMac::SetKiosk(bool kiosk) {
  if (kiosk && !is_kiosk_) {
    kiosk_options_ = [NSApp currentSystemPresentationOptions];
    NSApplicationPresentationOptions options =
        NSApplicationPresentationHideDock +
        NSApplicationPresentationHideMenuBar +
        NSApplicationPresentationDisableAppleMenu +
        NSApplicationPresentationDisableProcessSwitching +
        NSApplicationPresentationDisableForceQuit +
        NSApplicationPresentationDisableSessionTermination +
        NSApplicationPresentationDisableHideApplication;
    [NSApp setPresentationOptions:options];
    is_kiosk_ = true;
    SetFullScreen(true);
  } else if (!kiosk && is_kiosk_) {
    is_kiosk_ = false;
    SetFullScreen(false);
    [NSApp setPresentationOptions:kiosk_options_];
  }
}

bool NativeWindowMac::IsKiosk() {
  return is_kiosk_;
}

void NativeWindowMac::SetBackgroundColor(const std::string& color_name) {
  SkColor color = ParseHexColor(color_name);
  base::ScopedCFTypeRef<CGColorRef> cgcolor =
      skia::CGColorCreateFromSkColor(color);
  [[[window_ contentView] layer] setBackgroundColor:cgcolor];

  const auto view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->SetBackgroundColor(color);
}

void NativeWindowMac::SetHasShadow(bool has_shadow) {
  [window_ setHasShadow:has_shadow];
}

bool NativeWindowMac::HasShadow() {
  return [window_ hasShadow];
}

void NativeWindowMac::SetRepresentedFilename(const std::string& filename) {
  [window_ setRepresentedFilename:base::SysUTF8ToNSString(filename)];
}

std::string NativeWindowMac::GetRepresentedFilename() {
  return base::SysNSStringToUTF8([window_ representedFilename]);
}

void NativeWindowMac::SetDocumentEdited(bool edited) {
  [window_ setDocumentEdited:edited];
}

bool NativeWindowMac::IsDocumentEdited() {
  return [window_ isDocumentEdited];
}

void NativeWindowMac::SetIgnoreMouseEvents(bool ignore) {
  [window_ setIgnoresMouseEvents:ignore];
}

bool NativeWindowMac::HasModalDialog() {
  return [window_ attachedSheet] != nil;
}

gfx::NativeWindow NativeWindowMac::GetNativeWindow() {
  return window_;
}

gfx::AcceleratedWidget NativeWindowMac::GetAcceleratedWidget() {
  return inspectable_web_contents()->GetView()->GetNativeView();
}

void NativeWindowMac::SetProgressBar(double progress) {
  NSDockTile* dock_tile = [NSApp dockTile];

  // For the first time API invoked, we need to create a ContentView in DockTile.
  if (dock_tile.contentView == NULL) {
    NSImageView* image_view = [[NSImageView alloc] init];
    [image_view setImage:[NSApp applicationIconImage]];
    [dock_tile setContentView:image_view];

    NSProgressIndicator* progress_indicator = [[AtomProgressBar alloc]
        initWithFrame:NSMakeRect(0.0f, 0.0f, dock_tile.size.width, 15.0)];
    [progress_indicator setStyle:NSProgressIndicatorBarStyle];
    [progress_indicator setIndeterminate:NO];
    [progress_indicator setBezeled:YES];
    [progress_indicator setMinValue:0];
    [progress_indicator setMaxValue:1];
    [progress_indicator setHidden:NO];
    [image_view addSubview:progress_indicator];
  }

  NSProgressIndicator* progress_indicator =
      static_cast<NSProgressIndicator*>([[[dock_tile contentView] subviews]
           objectAtIndex:0]);
  if (progress < 0) {
    [progress_indicator setHidden:YES];
  } else if (progress > 1) {
    [progress_indicator setHidden:NO];
    [progress_indicator setIndeterminate:YES];
    [progress_indicator setDoubleValue:1];
  } else {
    [progress_indicator setHidden:NO];
    [progress_indicator setDoubleValue:progress];
  }
  [dock_tile display];
}

void NativeWindowMac::SetOverlayIcon(const gfx::Image& overlay,
                                     const std::string& description) {
}

void NativeWindowMac::ShowDefinitionForSelection() {
  if (!web_contents())
    return;
  auto rwhv = web_contents()->GetRenderWidgetHostView();
  if (!rwhv)
    return;
  rwhv->ShowDefinitionForSelection();
}

void NativeWindowMac::SetVisibleOnAllWorkspaces(bool visible) {
  SetCollectionBehavior(visible, NSWindowCollectionBehaviorCanJoinAllSpaces);
}

bool NativeWindowMac::IsVisibleOnAllWorkspaces() {
  NSUInteger collectionBehavior = [window_ collectionBehavior];
  return collectionBehavior & NSWindowCollectionBehaviorCanJoinAllSpaces;
}

std::vector<gfx::Rect> NativeWindowMac::CalculateNonDraggableRegions(
    const std::vector<DraggableRegion>& regions, int width, int height) {
  std::vector<gfx::Rect> result;
  if (regions.empty()) {
    result.push_back(gfx::Rect(0, 0, width, height));
  } else {
    scoped_ptr<SkRegion> draggable(DraggableRegionsToSkRegion(regions));
    scoped_ptr<SkRegion> non_draggable(new SkRegion);
    non_draggable->op(0, 0, width, height, SkRegion::kUnion_Op);
    non_draggable->op(*draggable, SkRegion::kDifference_Op);
    for (SkRegion::Iterator it(*non_draggable); !it.done(); it.next()) {
      result.push_back(gfx::SkIRectToRect(it.rect()));
    }
  }
  return result;
}

gfx::Size NativeWindowMac::ContentSizeToWindowSize(const gfx::Size& size) {
  if (!has_frame())
    return size;

  NSRect content = NSMakeRect(0, 0, size.width(), size.height());
  NSRect frame = [window_ frameRectForContentRect:content];
  return gfx::Size(frame.size);
}

gfx::Size NativeWindowMac::WindowSizeToContentSize(const gfx::Size& size) {
  if (!has_frame())
    return size;

  NSRect frame = NSMakeRect(0, 0, size.width(), size.height());
  NSRect content = [window_ contentRectForFrameRect:frame];
  return gfx::Size(content.size);
}

void NativeWindowMac::UpdateDraggableRegions(
    const std::vector<DraggableRegion>& regions) {
  NativeWindow::UpdateDraggableRegions(regions);
  draggable_regions_ = regions;
  UpdateDraggableRegionViews(regions);
}

void NativeWindowMac::InstallView() {
  // Make sure the bottom corner is rounded: http://crbug.com/396264.
  [[window_ contentView] setWantsLayer:YES];

  NSView* view = inspectable_web_contents()->GetView()->GetNativeView();
  if (has_frame()) {
    [view setFrame:[[window_ contentView] bounds]];
    [[window_ contentView] addSubview:view];
  } else {
    // In OSX 10.10, adding subviews to the root view for the NSView hierarchy
    // produces warnings. To eliminate the warnings, we resize the contentView
    // to fill the window, and add subviews to that.
    // http://crbug.com/380412
    content_view_.reset([[FullSizeContentView alloc] init]);
    [content_view_
        setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [content_view_ setFrame:[[[window_ contentView] superview] bounds]];
    [window_ setContentView:content_view_];

    [view setFrame:[content_view_ bounds]];
    [content_view_ addSubview:view];

    if (force_show_buttons_)
      return;

    // Hide the window buttons.
    [[window_ standardWindowButton:NSWindowZoomButton] setHidden:YES];
    [[window_ standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
    [[window_ standardWindowButton:NSWindowCloseButton] setHidden:YES];

    // Some third-party OS X utilities check the zoom button's enabled state to
    // determine whether to show custom UI on hover, so we disable it here to
    // prevent them from doing so in a frameless app window.
    [[window_ standardWindowButton:NSWindowZoomButton] setEnabled:NO];
  }
}

void NativeWindowMac::UninstallView() {
  NSView* view = inspectable_web_contents()->GetView()->GetNativeView();
  [view removeFromSuperview];
}

void NativeWindowMac::UpdateDraggableRegionViews(
    const std::vector<DraggableRegion>& regions) {
  if (has_frame())
    return;

  // All ControlRegionViews should be added as children of the WebContentsView,
  // because WebContentsView will be removed and re-added when entering and
  // leaving fullscreen mode.
  NSView* webView = web_contents()->GetNativeView();
  NSInteger webViewWidth = NSWidth([webView bounds]);
  NSInteger webViewHeight = NSHeight([webView bounds]);

  [webView setMouseDownCanMoveWindow:YES];

  // Remove all ControlRegionViews that are added last time.
  // Note that [webView subviews] returns the view's mutable internal array and
  // it should be copied to avoid mutating the original array while enumerating
  // it.
  base::scoped_nsobject<NSArray> subviews([[webView subviews] copy]);
  for (NSView* subview in subviews.get())
    if ([subview isKindOfClass:[ControlRegionView class]])
      [subview removeFromSuperview];

  // Draggable regions is implemented by having the whole web view draggable
  // (mouseDownCanMoveWindow) and overlaying regions that are not draggable.
  std::vector<gfx::Rect> system_drag_exclude_areas =
      CalculateNonDraggableRegions(regions, webViewWidth, webViewHeight);

  // Create and add a ControlRegionView for each region that needs to be
  // excluded from the dragging.
  for (std::vector<gfx::Rect>::const_iterator iter =
           system_drag_exclude_areas.begin();
       iter != system_drag_exclude_areas.end();
       ++iter) {
    base::scoped_nsobject<NSView> controlRegion(
        [[ControlRegionView alloc] initWithFrame:NSZeroRect]);
    [controlRegion setFrame:NSMakeRect(iter->x(),
                                       webViewHeight - iter->bottom(),
                                       iter->width(),
                                       iter->height())];
    [webView addSubview:controlRegion];
  }

  // AppKit will not update its cache of mouseDownCanMoveWindow unless something
  // changes. Previously we tried adding an NSView and removing it, but for some
  // reason it required reposting the mouse-down event, and didn't always work.
  // Calling the below seems to be an effective solution.
  [window_ setMovableByWindowBackground:NO];
  [window_ setMovableByWindowBackground:YES];
}

void NativeWindowMac::SetStyleMask(bool on, NSUInteger flag) {
  bool zoom_button_enabled = IsMaximizable();
  if (on)
    [window_ setStyleMask:[window_ styleMask] | flag];
  else
    [window_ setStyleMask:[window_ styleMask] & (~flag)];
  // Change style mask will make the zoom button revert to default, probably
  // a bug of Cocoa or OS X.
  if (!zoom_button_enabled)
    SetMaximizable(false);
}

void NativeWindowMac::SetCollectionBehavior(bool on, NSUInteger flag) {
  bool zoom_button_enabled = IsMaximizable();
  if (on)
    [window_ setCollectionBehavior:[window_ collectionBehavior] | flag];
  else
    [window_ setCollectionBehavior:[window_ collectionBehavior] & (~flag)];
  // Change collectionBehavior will make the zoom button revert to default,
  // probably a bug of Cocoa or OS X.
  if (!zoom_button_enabled)
    SetMaximizable(false);
}

// static
NativeWindow* NativeWindow::Create(
    brightray::InspectableWebContents* inspectable_web_contents,
    const mate::Dictionary& options) {
  return new NativeWindowMac(inspectable_web_contents, options);
}

}  // namespace atom
