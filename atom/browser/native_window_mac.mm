// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window_mac.h"

#include <string>

#import "atom/browser/ui/cocoa/event_processing_window.h"
#include "atom/common/draggable_region.h"
#include "atom/common/options_switches.h"
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "native_mate/dictionary.h"
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

    // If the new width is less than the frame size use it as the primary
    // constraint. This ensures that the value returned by this method will
    // never be larger than the users requested window size.
    if (newSize.width <= frameSize.width) {
      newSize.height =
          roundf((newSize.width - extraWidthPlusFrame) / aspectRatio +
                 extraHeightPlusFrame);
    } else {
      newSize.height =
          roundf((frameSize.width - extraWidthPlusFrame) / aspectRatio +
                 extraHeightPlusFrame);
      newSize.width =
          roundf((newSize.height - extraHeightPlusFrame) * aspectRatio +
                 extraWidthPlusFrame);
    }
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

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
  shell_->NotifyWindowEnterFullScreen();
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
  if (!shell_->has_frame()) {
    NSWindow* window = shell_->GetNativeWindow();
    [[window standardWindowButton:NSWindowFullScreenButton] setHidden:YES];
  }

  shell_->NotifyWindowLeaveFullScreen();
}

- (void)windowWillClose:(NSNotification*)notification {
  shell_->NotifyWindowClosed();
}

- (BOOL)windowShouldClose:(id)window {
  // When user tries to close the window by clicking the close button, we do
  // not close the window immediately, instead we try to close the web page
  // fisrt, and when the web page is closed the window will also be closed.
  shell_->RequestToClosePage();
  return NO;
}

@end

@interface AtomNSWindow : EventProcessingWindow {
 @private
  atom::NativeWindowMac* shell_;
  bool enable_larger_than_screen_;
}
@property BOOL acceptsFirstMouse;
@property BOOL disableAutoHideCursor;
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
      attention_request_id_(0) {
  int width = 800, height = 600;
  options.Get(switches::kWidth, &width);
  options.Get(switches::kHeight, &height);

  NSRect main_screen_rect = [[[NSScreen screens] objectAtIndex:0] frame];
  NSRect cocoa_bounds = NSMakeRect(
      round((NSWidth(main_screen_rect) - width) / 2) ,
      round((NSHeight(main_screen_rect) - height) / 2),
      width,
      height);

  bool useStandardWindow = true;
  options.Get(switches::kStandardWindow, &useStandardWindow);
  bool resizable = true;
  options.Get(switches::kResizable, &resizable);

  // New title bar styles are available in Yosemite or newer
  std::string titleBarStyle;
  if (base::mac::IsOSYosemiteOrLater())
    options.Get(switches::kTitleBarStyle, &titleBarStyle);

  NSUInteger styleMask = NSTitledWindowMask | NSClosableWindowMask |
                         NSMiniaturizableWindowMask;
  if (!useStandardWindow || transparent() || !has_frame()) {
    styleMask |= NSTexturedBackgroundWindowMask;
  }
  if (resizable) {
    styleMask |= NSResizableWindowMask;
  }
  if ((titleBarStyle == "hidden") || (titleBarStyle == "hidden-inset")) {
    styleMask |= NSFullSizeContentViewWindowMask;
    styleMask |= NSUnifiedTitleAndToolbarWindowMask;
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
    [window_ setHasShadow:NO];
    [window_ setBackgroundColor:[NSColor clearColor]];
  }

  // Remove non-transparent corners, see http://git.io/vfonD.
  if (!has_frame())
    [window_ setOpaque:NO];

  // We will manage window's lifetime ourselves.
  [window_ setReleasedWhenClosed:NO];

  // Hide the title bar.
  if ((titleBarStyle == "hidden") || (titleBarStyle == "hidden-inset")) {
    [window_ setTitlebarAppearsTransparent:YES];
    [window_ setTitleVisibility:NSWindowTitleHidden];
    if (titleBarStyle == "hidden-inset") {
      base::scoped_nsobject<NSToolbar> toolbar(
          [[NSToolbar alloc] initWithIdentifier:@"titlebarStylingToolbar"]);
      [toolbar setShowsBaselineSeparator:NO];
      [window_ setToolbar:toolbar];
    }
    // We should be aware of draggable regions when using hidden titlebar.
    set_force_using_draggable_region(true);
  }

  // On OS X the initial window size doesn't include window frame.
  bool use_content_size = false;
  options.Get(switches::kUseContentSize, &use_content_size);
  if (!has_frame() || !use_content_size)
    SetSize(gfx::Size(width, height));

  // Enable the NSView to accept first mouse event.
  bool acceptsFirstMouse = false;
  options.Get(switches::kAcceptFirstMouse, &acceptsFirstMouse);
  [window_ setAcceptsFirstMouse:acceptsFirstMouse];

  // Disable auto-hiding cursor.
  bool disableAutoHideCursor = false;
  options.Get(switches::kDisableAutoHideCursor, &disableAutoHideCursor);
  [window_ setDisableAutoHideCursor:disableAutoHideCursor];

  // Disable fullscreen button when 'fullscreen' is specified to false.
  bool fullscreen;
  if (!(options.Get(switches::kFullscreen, &fullscreen) &&
        !fullscreen)) {
    NSUInteger collectionBehavior = [window_ collectionBehavior];
    collectionBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;
    [window_ setCollectionBehavior:collectionBehavior];
  }

  NSView* view = inspectable_web_contents()->GetView()->GetNativeView();
  [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

  InstallView();
}

NativeWindowMac::~NativeWindowMac() {
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
  [window_ zoom:nil];
}

void NativeWindowMac::Unmaximize() {
  [window_ zoom:nil];
}

bool NativeWindowMac::IsMaximized() {
  return [window_ isZoomed];
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

void NativeWindowMac::SetBounds(const gfx::Rect& bounds) {
  NSRect cocoa_bounds = NSMakeRect(bounds.x(), 0,
                                   bounds.width(),
                                   bounds.height());
  // Flip coordinates based on the primary screen.
  NSScreen* screen = [[NSScreen screens] objectAtIndex:0];
  cocoa_bounds.origin.y =
      NSHeight([screen frame]) - bounds.height() - bounds.y();

  [window_ setFrame:cocoa_bounds display:YES];
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
  if (resizable) {
    [[window_ standardWindowButton:NSWindowZoomButton] setEnabled:YES];
    [window_ setStyleMask:[window_ styleMask] | NSResizableWindowMask];
  } else {
    [[window_ standardWindowButton:NSWindowZoomButton] setEnabled:NO];
    [window_ setStyleMask:[window_ styleMask] & (~NSResizableWindowMask)];
  }
}

bool NativeWindowMac::IsResizable() {
  return [window_ styleMask] & NSResizableWindowMask;
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

bool NativeWindowMac::HasModalDialog() {
  return [window_ attachedSheet] != nil;
}

gfx::NativeWindow NativeWindowMac::GetNativeWindow() {
  return window_;
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
  NSUInteger collectionBehavior = [window_ collectionBehavior];
  if (visible) {
    collectionBehavior |= NSWindowCollectionBehaviorCanJoinAllSpaces;
  } else {
    collectionBehavior &= ~NSWindowCollectionBehaviorCanJoinAllSpaces;
  }
  [window_ setCollectionBehavior:collectionBehavior];
}

bool NativeWindowMac::IsVisibleOnAllWorkspaces() {
  NSUInteger collectionBehavior = [window_ collectionBehavior];
  return collectionBehavior & NSWindowCollectionBehaviorCanJoinAllSpaces;
}

void NativeWindowMac::HandleKeyboardEvent(
    content::WebContents*,
    const content::NativeWebKeyboardEvent& event) {
  if (event.skip_in_browser ||
      event.type == content::NativeWebKeyboardEvent::Char)
    return;

  if (event.os_event.window == window_.get()) {
    EventProcessingWindow* event_window =
        static_cast<EventProcessingWindow*>(window_);
    DCHECK([event_window isKindOfClass:[EventProcessingWindow class]]);
    [event_window redispatchKeyEvent:event.os_event];
  } else {
    // The event comes from detached devtools view, and it has already been
    // handled by the devtools itself, we now send it to application menu to
    // make menu acclerators work.
    BOOL handled = [[NSApp mainMenu] performKeyEquivalent:event.os_event];
    // Handle the cmd+~ shortcut.
    if (!handled && (event.os_event.modifierFlags & NSCommandKeyMask) &&
        (event.os_event.keyCode == 50  /* ~ key */))
      Focus(true);
  }
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
  if (has_frame() && !force_using_draggable_region())
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

// static
NativeWindow* NativeWindow::Create(
    brightray::InspectableWebContents* inspectable_web_contents,
    const mate::Dictionary& options) {
  return new NativeWindowMac(inspectable_web_contents, options);
}

}  // namespace atom
