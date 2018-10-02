// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window_mac.h"

#include <AvailabilityMacros.h>
#include <objc/objc-runtime.h>

#include <string>

#include "atom/browser/native_browser_view_mac.h"
#include "atom/browser/ui/cocoa/atom_native_widget_mac.h"
#include "atom/browser/ui/cocoa/atom_ns_window.h"
#include "atom/browser/ui/cocoa/atom_ns_window_delegate.h"
#include "atom/browser/ui/cocoa/atom_preview_item.h"
#include "atom/browser/ui/cocoa/atom_touch_bar.h"
#include "atom/browser/ui/cocoa/root_view_mac.h"
#include "atom/browser/window_list.h"
#include "atom/common/options_switches.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/strings/sys_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "native_mate/dictionary.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/skia_util.h"
#include "ui/gl/gpu_switching_manager.h"
#include "ui/views/background.h"
#include "ui/views/cocoa/bridged_native_widget.h"
#include "ui/views/widget/widget.h"

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

// Custom Quit, Minimize and Full Screen button container for frameless
// windows.
@interface CustomWindowButtonView : NSView {
 @private
  BOOL mouse_inside_;
}
@end

@implementation CustomWindowButtonView

- (id)initWithFrame:(NSRect)frame {
  self = [super initWithFrame:frame];

  NSButton* close_button =
      [NSWindow standardWindowButton:NSWindowCloseButton
                        forStyleMask:NSWindowStyleMaskTitled];
  NSButton* miniaturize_button =
      [NSWindow standardWindowButton:NSWindowMiniaturizeButton
                        forStyleMask:NSWindowStyleMaskTitled];
  NSButton* zoom_button =
      [NSWindow standardWindowButton:NSWindowZoomButton
                        forStyleMask:NSWindowStyleMaskTitled];

  CGFloat x = 0;
  const CGFloat space_between = 20;

  [close_button setFrameOrigin:NSMakePoint(x, 0)];
  x += space_between;
  [self addSubview:close_button];

  [miniaturize_button setFrameOrigin:NSMakePoint(x, 0)];
  x += space_between;
  [self addSubview:miniaturize_button];

  [zoom_button setFrameOrigin:NSMakePoint(x, 0)];
  x += space_between;
  [self addSubview:zoom_button];

  const auto last_button_frame = zoom_button.frame;
  [self setFrameSize:NSMakeSize(last_button_frame.origin.x +
                                    last_button_frame.size.width,
                                last_button_frame.size.height)];

  mouse_inside_ = NO;
  [self setNeedsDisplayForButtons];

  return self;
}

- (void)viewDidMoveToWindow {
  if (!self.window) {
    return;
  }

  // Stay in upper left corner.
  const CGFloat top_margin = 3;
  const CGFloat left_margin = 7;
  [self setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];
  [self setFrameOrigin:NSMakePoint(left_margin, self.window.frame.size.height -
                                                    self.frame.size.height -
                                                    top_margin)];
}

- (BOOL)_mouseInGroup:(NSButton*)button {
  return mouse_inside_;
}

- (void)updateTrackingAreas {
  auto tracking_area = [[[NSTrackingArea alloc]
      initWithRect:NSZeroRect
           options:NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways |
                   NSTrackingInVisibleRect
             owner:self
          userInfo:nil] autorelease];
  [self addTrackingArea:tracking_area];
}

- (void)mouseEntered:(NSEvent*)event {
  [super mouseEntered:event];
  mouse_inside_ = YES;
  [self setNeedsDisplayForButtons];
}

- (void)mouseExited:(NSEvent*)event {
  [super mouseExited:event];
  mouse_inside_ = NO;
  [self setNeedsDisplayForButtons];
}

- (void)setNeedsDisplayForButtons {
  for (NSView* subview in self.subviews) {
    [subview setHidden:!mouse_inside_];
    [subview setNeedsDisplay:YES];
  }
}

@end

#if !defined(AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER)

enum { NSWindowTabbingModeDisallowed = 2 };

@interface NSWindow (SierraSDK)
- (void)setTabbingMode:(NSInteger)mode;
- (void)setTabbingIdentifier:(NSString*)identifier;
- (void)addTabbedWindow:(NSWindow*)window ordered:(NSWindowOrderingMode)ordered;
- (IBAction)selectPreviousTab:(id)sender;
- (IBAction)selectNextTab:(id)sender;
- (IBAction)mergeAllWindows:(id)sender;
- (IBAction)moveTabToNewWindow:(id)sender;
- (IBAction)toggleTabBar:(id)sender;
@end

#endif

@interface AtomProgressBar : NSProgressIndicator
@end

@implementation AtomProgressBar

- (void)drawRect:(NSRect)dirtyRect {
  if (self.style != NSProgressIndicatorBarStyle)
    return;
  // Draw edges of rounded rect.
  NSRect rect = NSInsetRect([self bounds], 1.0, 1.0);
  CGFloat radius = rect.size.height / 2;
  NSBezierPath* bezier_path = [NSBezierPath bezierPathWithRoundedRect:rect
                                                              xRadius:radius
                                                              yRadius:radius];
  [bezier_path setLineWidth:2.0];
  [[NSColor grayColor] set];
  [bezier_path stroke];

  // Fill the rounded rect.
  rect = NSInsetRect(rect, 2.0, 2.0);
  radius = rect.size.height / 2;
  bezier_path = [NSBezierPath bezierPathWithRoundedRect:rect
                                                xRadius:radius
                                                yRadius:radius];
  [bezier_path setLineWidth:1.0];
  [bezier_path addClip];

  // Calculate the progress width.
  rect.size.width =
      floor(rect.size.width * ([self doubleValue] / [self maxValue]));

  // Fill the progress bar with color blue.
  [[NSColor colorWithSRGBRed:0.2 green:0.6 blue:1 alpha:1] set];
  NSRectFill(rect);
}

@end

namespace mate {

template <>
struct Converter<atom::NativeWindowMac::TitleBarStyle> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     atom::NativeWindowMac::TitleBarStyle* out) {
    std::string title_bar_style;
    if (!ConvertFromV8(isolate, val, &title_bar_style))
      return false;
    if (title_bar_style == "hidden") {
      *out = atom::NativeWindowMac::HIDDEN;
    } else if (title_bar_style == "hiddenInset") {
      *out = atom::NativeWindowMac::HIDDEN_INSET;
    } else if (title_bar_style == "customButtonsOnHover") {
      *out = atom::NativeWindowMac::CUSTOM_BUTTONS_ON_HOVER;
    } else {
      return false;
    }
    return true;
  }
};

}  // namespace mate

namespace atom {

namespace {

bool IsFramelessWindow(NSView* view) {
  NativeWindow* window = [static_cast<AtomNSWindow*>([view window]) shell];
  return window && !window->has_frame();
}

IMP original_set_frame_size = nullptr;
IMP original_view_did_move_to_superview = nullptr;

// This method is directly called by NSWindow during a window resize on OSX
// 10.10.0, beta 2. We must override it to prevent the content view from
// shrinking.
void SetFrameSize(NSView* self, SEL _cmd, NSSize size) {
  if (!IsFramelessWindow(self)) {
    auto original =
        reinterpret_cast<decltype(&SetFrameSize)>(original_set_frame_size);
    return original(self, _cmd, size);
  }
  // For frameless window, resize the view to cover full window.
  if ([self superview])
    size = [[self superview] bounds].size;
  auto super_impl = reinterpret_cast<decltype(&SetFrameSize)>(
      [[self superclass] instanceMethodForSelector:_cmd]);
  super_impl(self, _cmd, size);
}

// The contentView gets moved around during certain full-screen operations.
// This is less than ideal, and should eventually be removed.
void ViewDidMoveToSuperview(NSView* self, SEL _cmd) {
  if (!IsFramelessWindow(self)) {
    // [BridgedContentView viewDidMoveToSuperview];
    auto original = reinterpret_cast<decltype(&ViewDidMoveToSuperview)>(
        original_view_did_move_to_superview);
    if (original)
      original(self, _cmd);
    return;
  }
  [self setFrame:[[self superview] bounds]];
}

}  // namespace

NativeWindowMac::NativeWindowMac(const mate::Dictionary& options,
                                 NativeWindow* parent)
    : NativeWindow(options, parent), root_view_(new RootViewMac(this)) {
  int width = 800, height = 600;
  options.Get(options::kWidth, &width);
  options.Get(options::kHeight, &height);

  NSRect main_screen_rect = [[[NSScreen screens] firstObject] frame];
  gfx::Rect bounds(round((NSWidth(main_screen_rect) - width) / 2),
                   round((NSHeight(main_screen_rect) - height) / 2), width,
                   height);

  options.Get(options::kResizable, &resizable_);
  options.Get(options::kTitleBarStyle, &title_bar_style_);
  options.Get(options::kZoomToPageWidth, &zoom_to_page_width_);
  options.Get(options::kFullscreenWindowTitle, &fullscreen_window_title_);
  options.Get(options::kSimpleFullScreen, &always_simple_fullscreen_);

  bool minimizable = true;
  options.Get(options::kMinimizable, &minimizable);

  bool maximizable = true;
  options.Get(options::kMaximizable, &maximizable);

  bool closable = true;
  options.Get(options::kClosable, &closable);

  std::string tabbingIdentifier;
  options.Get(options::kTabbingIdentifier, &tabbingIdentifier);

  std::string windowType;
  options.Get(options::kType, &windowType);

  bool useStandardWindow = true;
  // eventually deprecate separate "standardWindow" option in favor of
  // standard / textured window types
  options.Get(options::kStandardWindow, &useStandardWindow);
  if (windowType == "textured") {
    useStandardWindow = false;
  }

  NSUInteger styleMask = NSWindowStyleMaskTitled;
  if (@available(macOS 10.10, *)) {
    if (title_bar_style_ == CUSTOM_BUTTONS_ON_HOVER &&
        (!useStandardWindow || transparent() || !has_frame())) {
      styleMask = NSWindowStyleMaskFullSizeContentView;
    }
  }
  if (minimizable) {
    styleMask |= NSMiniaturizableWindowMask;
  }
  if (closable) {
    styleMask |= NSWindowStyleMaskClosable;
  }
  if (title_bar_style_ != NORMAL) {
    // The window without titlebar is treated the same with frameless window.
    set_has_frame(false);
  }
  if (!useStandardWindow || transparent() || !has_frame()) {
    styleMask |= NSTexturedBackgroundWindowMask;
  }

  // Create views::Widget and assign window_ with it.
  // TODO(zcbenz): Get rid of the window_ in future.
  views::Widget::InitParams params;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.bounds = bounds;
  params.delegate = this;
  params.type = views::Widget::InitParams::TYPE_WINDOW;
  params.native_widget = new AtomNativeWidgetMac(this, styleMask, widget());
  widget()->Init(params);
  window_ = static_cast<AtomNSWindow*>(widget()->GetNativeWindow());

  [window_ setEnableLargerThanScreen:enable_larger_than_screen()];

  window_delegate_.reset([[AtomNSWindowDelegate alloc] initWithShell:this]);
  [window_ setDelegate:window_delegate_];

  // Only use native parent window for non-modal windows.
  if (parent && !is_modal()) {
    SetParentWindow(parent);
  }

  if (transparent()) {
    // Setting the background color to clear will also hide the shadow.
    [window_ setBackgroundColor:[NSColor clearColor]];
  }

  if (windowType == "desktop") {
    [window_ setLevel:kCGDesktopWindowLevel - 1];
    [window_ setDisableKeyOrMainWindow:YES];
    [window_ setCollectionBehavior:(NSWindowCollectionBehaviorCanJoinAllSpaces |
                                    NSWindowCollectionBehaviorStationary |
                                    NSWindowCollectionBehaviorIgnoresCycle)];
  }

  bool focusable;
  if (options.Get(options::kFocusable, &focusable) && !focusable)
    [window_ setDisableKeyOrMainWindow:YES];

  if (transparent() || !has_frame()) {
    if (@available(macOS 10.10, *)) {
      // Don't show title bar.
      [window_ setTitlebarAppearsTransparent:YES];
      [window_ setTitleVisibility:NSWindowTitleHidden];
    }
    // Remove non-transparent corners, see http://git.io/vfonD.
    [window_ setOpaque:NO];
  }

  // Create a tab only if tabbing identifier is specified and window has
  // a native title bar.
  if (tabbingIdentifier.empty() || transparent() || !has_frame()) {
    if (@available(macOS 10.12, *)) {
      [window_ setTabbingMode:NSWindowTabbingModeDisallowed];
    }
  } else {
    if (@available(macOS 10.12, *)) {
      [window_ setTabbingIdentifier:base::SysUTF8ToNSString(tabbingIdentifier)];
    }
  }

  // Hide the title bar background
  if (title_bar_style_ != NORMAL) {
    if (@available(macOS 10.10, *)) {
      [window_ setTitlebarAppearsTransparent:YES];
    }
  }

  // Hide the title bar.
  if (title_bar_style_ == HIDDEN_INSET) {
    if (@available(macOS 10.10, *)) {
      base::scoped_nsobject<NSToolbar> toolbar(
          [[NSToolbar alloc] initWithIdentifier:@"titlebarStylingToolbar"]);
      [toolbar setShowsBaselineSeparator:NO];
      [window_ setToolbar:toolbar];
    } else {
      [window_ enableWindowButtonsOffset];
      [window_ setWindowButtonsOffset:NSMakePoint(12, 10)];
    }
  }

  // Resize to content bounds.
  bool use_content_size = false;
  options.Get(options::kUseContentSize, &use_content_size);
  if (!has_frame() || use_content_size)
    SetContentSize(gfx::Size(width, height));

  // Enable the NSView to accept first mouse event.
  bool acceptsFirstMouse = false;
  options.Get(options::kAcceptFirstMouse, &acceptsFirstMouse);
  [window_ setAcceptsFirstMouse:acceptsFirstMouse];

  // Disable auto-hiding cursor.
  bool disableAutoHideCursor = false;
  options.Get(options::kDisableAutoHideCursor, &disableAutoHideCursor);
  [window_ setDisableAutoHideCursor:disableAutoHideCursor];

  // Use an NSEvent monitor to listen for the wheel event.
  BOOL __block began = NO;
  wheel_event_monitor_ = [NSEvent
      addLocalMonitorForEventsMatchingMask:NSScrollWheelMask
                                   handler:^(NSEvent* event) {
                                     if ([[event window] windowNumber] !=
                                         [window_ windowNumber])
                                       return event;

                                     if (!began && (([event phase] ==
                                                     NSEventPhaseMayBegin) ||
                                                    ([event phase] ==
                                                     NSEventPhaseBegan))) {
                                       this->NotifyWindowScrollTouchBegin();
                                       began = YES;
                                     } else if (began &&
                                                (([event phase] ==
                                                  NSEventPhaseEnded) ||
                                                 ([event phase] ==
                                                  NSEventPhaseCancelled))) {
                                       this->NotifyWindowScrollTouchEnd();
                                       began = NO;
                                     }
                                     return event;
                                   }];

  // Set maximizable state last to ensure zoom button does not get reset
  // by calls to other APIs.
  SetMaximizable(maximizable);

  // Default content view.
  SetContentView(new views::View());
  AddContentViewLayers();

  original_frame_ = [window_ frame];
}

NativeWindowMac::~NativeWindowMac() {
  [NSEvent removeMonitor:wheel_event_monitor_];
}

void NativeWindowMac::SetContentView(views::View* view) {
  views::View* root_view = GetContentsView();
  if (content_view())
    root_view->RemoveChildView(content_view());

  set_content_view(view);
  root_view->AddChildView(content_view());

  if (buttons_view_) {
    // Ensure the buttons view are always floated on the top.
    [buttons_view_ removeFromSuperview];
    [[window_ contentView] addSubview:buttons_view_];
  }

  root_view->Layout();
}

void NativeWindowMac::Close() {
  // When this is a sheet showing, performClose won't work.
  if (is_modal() && parent() && IsVisible()) {
    [parent()->GetNativeWindow() endSheet:window_];
    CloseImmediately();
    return;
  }

  if (!IsClosable()) {
    WindowList::WindowCloseCancelled(this);
    return;
  }

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
  if (is_modal() && parent()) {
    if ([window_ sheetParent] == nil)
      [parent()->GetNativeWindow() beginSheet:window_
                            completionHandler:^(NSModalResponse){
                            }];
    return;
  }

  // Reattach the window to the parent to actually show it.
  if (parent())
    InternalSetParentWindow(parent(), true);

  // This method is supposed to put focus on window, however if the app does not
  // have focus then "makeKeyAndOrderFront" will only show the window.
  [NSApp activateIgnoringOtherApps:YES];

  [window_ makeKeyAndOrderFront:nil];
}

void NativeWindowMac::ShowInactive() {
  // Reattach the window to the parent to actually show it.
  if (parent())
    InternalSetParentWindow(parent(), true);

  [window_ orderFrontRegardless];
}

void NativeWindowMac::Hide() {
  if (is_modal() && parent()) {
    [window_ orderOut:nil];
    [parent()->GetNativeWindow() endSheet:window_];
    return;
  }

  // Deattach the window from the parent before.
  if (parent())
    InternalSetParentWindow(parent(), false);

  [window_ orderOut:nil];
}

bool NativeWindowMac::IsVisible() {
  return [window_ isVisible];
}

bool NativeWindowMac::IsEnabled() {
  return [window_ attachedSheet] == nil;
}

void NativeWindowMac::SetEnabled(bool enable) {
  if (enable) {
    [window_ beginSheet:window_
        completionHandler:^(NSModalResponse returnCode) {
          NSLog(@"modal enabled");
          return;
        }];
  } else {
    [window_ endSheet:[window_ attachedSheet]];
  }
}

void NativeWindowMac::Maximize() {
  if (IsMaximized())
    return;

  // Take note of the current window size
  if (IsNormal())
    original_frame_ = [window_ frame];
  [window_ zoom:nil];
}

void NativeWindowMac::Unmaximize() {
  if (!IsMaximized())
    return;

  [window_ zoom:nil];
}

bool NativeWindowMac::IsMaximized() {
  if (([window_ styleMask] & NSWindowStyleMaskResizable) != 0) {
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
  if (IsMinimized())
    return;

  // Take note of the current window size
  if (IsNormal())
    original_frame_ = [window_ frame];
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

  // Take note of the current window size
  if (IsNormal())
    original_frame_ = [window_ frame];
  [window_ toggleFullScreenMode:nil];
}

bool NativeWindowMac::IsFullscreen() const {
  return [window_ styleMask] & NSWindowStyleMaskFullScreen;
}

void NativeWindowMac::SetBounds(const gfx::Rect& bounds, bool animate) {
  // Do nothing if in fullscreen mode.
  if (IsFullscreen())
    return;

  // Check size constraints since setFrame does not check it.
  gfx::Size size = bounds.size();
  size.SetToMax(GetMinimumSize());
  gfx::Size max_size = GetMaximumSize();
  if (!max_size.IsEmpty())
    size.SetToMin(max_size);

  NSRect cocoa_bounds = NSMakeRect(bounds.x(), 0, size.width(), size.height());
  // Flip coordinates based on the primary screen.
  NSScreen* screen = [[NSScreen screens] firstObject];
  cocoa_bounds.origin.y = NSHeight([screen frame]) - size.height() - bounds.y();

  [window_ setFrame:cocoa_bounds display:YES animate:animate];
}

gfx::Rect NativeWindowMac::GetBounds() {
  NSRect frame = [window_ frame];
  gfx::Rect bounds(frame.origin.x, 0, NSWidth(frame), NSHeight(frame));
  NSScreen* screen = [[NSScreen screens] firstObject];
  bounds.set_y(NSHeight([screen frame]) - NSMaxY(frame));
  return bounds;
}

bool NativeWindowMac::IsNormal() {
  return NativeWindow::IsNormal() && !IsSimpleFullScreen();
}

gfx::Rect NativeWindowMac::GetNormalBounds() {
  if (IsNormal()) {
    return GetBounds();
  }
  NSRect frame = original_frame_;
  gfx::Rect bounds(frame.origin.x, 0, NSWidth(frame), NSHeight(frame));
  NSScreen* screen = [[NSScreen screens] firstObject];
  bounds.set_y(NSHeight([screen frame]) - NSMaxY(frame));
  return bounds;
  // Works on OS_WIN !
  // return widget()->GetRestoredBounds();
}

void NativeWindowMac::SetContentSizeConstraints(
    const extensions::SizeConstraints& size_constraints) {
  auto convertSize = [this](const gfx::Size& size) {
    // Our frameless window still has titlebar attached, so setting contentSize
    // will result in actual content size being larger.
    if (!has_frame()) {
      NSRect frame = NSMakeRect(0, 0, size.width(), size.height());
      NSRect content = [window_ originalContentRectForFrameRect:frame];
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

void NativeWindowMac::MoveTop() {
  [window_ orderWindow:NSWindowAbove relativeTo:0];
}

void NativeWindowMac::SetResizable(bool resizable) {
  SetStyleMask(resizable, NSWindowStyleMaskResizable);
}

bool NativeWindowMac::IsResizable() {
  return [window_ styleMask] & NSWindowStyleMaskResizable;
}

void NativeWindowMac::SetAspectRatio(double aspect_ratio,
                                     const gfx::Size& extra_size) {
  NativeWindow::SetAspectRatio(aspect_ratio, extra_size);

  // Reset the behaviour to default if aspect_ratio is set to 0 or less.
  if (aspect_ratio > 0.0)
    [window_ setAspectRatio:NSMakeSize(aspect_ratio, 1.0)];
  else
    [window_ setResizeIncrements:NSMakeSize(1.0, 1.0)];
}

void NativeWindowMac::PreviewFile(const std::string& path,
                                  const std::string& display_name) {
  preview_item_.reset([[AtomPreviewItem alloc]
      initWithURL:[NSURL fileURLWithPath:base::SysUTF8ToNSString(path)]
            title:base::SysUTF8ToNSString(display_name)]);
  [[QLPreviewPanel sharedPreviewPanel] makeKeyAndOrderFront:nil];
}

void NativeWindowMac::CloseFilePreview() {
  if ([QLPreviewPanel sharedPreviewPanelExists]) {
    [[QLPreviewPanel sharedPreviewPanel] close];
  }
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
  SetCollectionBehavior(fullscreenable,
                        NSWindowCollectionBehaviorFullScreenPrimary);
  // On EL Capitan this flag is required to hide fullscreen button.
  SetCollectionBehavior(!fullscreenable,
                        NSWindowCollectionBehaviorFullScreenAuxiliary);
}

bool NativeWindowMac::IsFullScreenable() {
  NSUInteger collectionBehavior = [window_ collectionBehavior];
  return collectionBehavior & NSWindowCollectionBehaviorFullScreenPrimary;
}

void NativeWindowMac::SetClosable(bool closable) {
  SetStyleMask(closable, NSWindowStyleMaskClosable);
}

bool NativeWindowMac::IsClosable() {
  return [window_ styleMask] & NSWindowStyleMaskClosable;
}

void NativeWindowMac::SetAlwaysOnTop(bool top,
                                     const std::string& level,
                                     int relativeLevel,
                                     std::string* error) {
  int windowLevel = NSNormalWindowLevel;
  CGWindowLevel maxWindowLevel = CGWindowLevelForKey(kCGMaximumWindowLevelKey);
  CGWindowLevel minWindowLevel = CGWindowLevelForKey(kCGMinimumWindowLevelKey);

  if (top) {
    if (level == "floating") {
      windowLevel = NSFloatingWindowLevel;
    } else if (level == "torn-off-menu") {
      windowLevel = NSTornOffMenuWindowLevel;
    } else if (level == "modal-panel") {
      windowLevel = NSModalPanelWindowLevel;
    } else if (level == "main-menu") {
      windowLevel = NSMainMenuWindowLevel;
    } else if (level == "status") {
      windowLevel = NSStatusWindowLevel;
    } else if (level == "pop-up-menu") {
      windowLevel = NSPopUpMenuWindowLevel;
    } else if (level == "screen-saver") {
      windowLevel = NSScreenSaverWindowLevel;
    } else if (level == "dock") {
      // Deprecated by macOS, but kept for backwards compatibility
      windowLevel = NSDockWindowLevel;
    }
  }

  NSInteger newLevel = windowLevel + relativeLevel;
  if (newLevel >= minWindowLevel && newLevel <= maxWindowLevel) {
    [window_ setLevel:newLevel];
  } else {
    *error = std::string([
        [NSString stringWithFormat:@"relativeLevel must be between %d and %d",
                                   minWindowLevel, maxWindowLevel] UTF8String]);
  }
}

bool NativeWindowMac::IsAlwaysOnTop() {
  return [window_ level] != NSNormalWindowLevel;
}

void NativeWindowMac::Center() {
  [window_ center];
}

void NativeWindowMac::Invalidate() {
  [window_ flushWindow];
  [[window_ contentView] setNeedsDisplay:YES];
}

void NativeWindowMac::SetTitle(const std::string& title) {
  // For macOS <= 10.9, the setTitleVisibility API is not available, we have
  // to avoid calling setTitle for frameless window.
  if (!base::mac::IsAtLeastOS10_10() && (transparent() || !has_frame()))
    return;

  [window_ setTitle:base::SysUTF8ToNSString(title)];
}

std::string NativeWindowMac::GetTitle() {
  return base::SysNSStringToUTF8([window_ title]);
  ;
}

void NativeWindowMac::FlashFrame(bool flash) {
  if (flash) {
    attention_request_id_ = [NSApp requestUserAttention:NSInformationalRequest];
  } else {
    [NSApp cancelUserAttentionRequest:attention_request_id_];
    attention_request_id_ = 0;
  }
}

void NativeWindowMac::SetSkipTaskbar(bool skip) {}

void NativeWindowMac::SetSimpleFullScreen(bool simple_fullscreen) {
  NSWindow* window = GetNativeWindow();

  if (simple_fullscreen && !is_simple_fullscreen_) {
    is_simple_fullscreen_ = true;

    // Take note of the current window size
    if (IsNormal())
      original_frame_ = [window_ frame];

    simple_fullscreen_options_ = [NSApp currentSystemPresentationOptions];
    simple_fullscreen_mask_ = [window styleMask];

    // We can simulate the pre-Lion fullscreen by auto-hiding the dock and menu
    // bar
    NSApplicationPresentationOptions options =
        NSApplicationPresentationAutoHideDock +
        NSApplicationPresentationAutoHideMenuBar;
    [NSApp setPresentationOptions:options];

    was_maximizable_ = IsMaximizable();
    was_movable_ = IsMovable();

    NSRect fullscreenFrame = [window.screen frame];

    if (!fullscreen_window_title()) {
      // Hide the titlebar
      SetStyleMask(false, NSWindowStyleMaskTitled);

      // Resize the window to accomodate the _entire_ screen size
      fullscreenFrame.size.height -=
          [[[NSApplication sharedApplication] mainMenu] menuBarHeight];
    } else if (!window_button_visibility_.has_value()) {
      // Lets keep previous behaviour - hide window controls in titled
      // fullscreen mode when not specified otherwise.
      [[window standardWindowButton:NSWindowZoomButton] setHidden:YES];
      [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
      [[window standardWindowButton:NSWindowCloseButton] setHidden:YES];
    }

    [window setFrame:fullscreenFrame display:YES animate:YES];

    // Fullscreen windows can't be resized, minimized, maximized, or moved
    SetMinimizable(false);
    SetResizable(false);
    SetMaximizable(false);
    SetMovable(false);
  } else if (!simple_fullscreen && is_simple_fullscreen_) {
    is_simple_fullscreen_ = false;

    if (!fullscreen_window_title()) {
      // Restore the titlebar
      SetStyleMask(true, NSWindowStyleMaskTitled);
    }

    // Restore window controls visibility state
    const bool window_button_hidden = !window_button_visibility_.value_or(true);
    [[window standardWindowButton:NSWindowZoomButton]
        setHidden:window_button_hidden];
    [[window standardWindowButton:NSWindowMiniaturizeButton]
        setHidden:window_button_hidden];
    [[window standardWindowButton:NSWindowCloseButton]
        setHidden:window_button_hidden];

    [window setFrame:original_frame_ display:YES animate:YES];

    [NSApp setPresentationOptions:simple_fullscreen_options_];

    // Restore original style mask
    ScopedDisableResize disable_resize;
    [window_ setStyleMask:simple_fullscreen_mask_];

    // Restore window manipulation abilities
    SetMaximizable(was_maximizable_);
    SetMovable(was_movable_);
  }
}

bool NativeWindowMac::IsSimpleFullScreen() {
  return is_simple_fullscreen_;
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
    was_fullscreen_ = IsFullscreen();
    if (!was_fullscreen_)
      SetFullScreen(true);
  } else if (!kiosk && is_kiosk_) {
    is_kiosk_ = false;
    if (!was_fullscreen_)
      SetFullScreen(false);
    [NSApp setPresentationOptions:kiosk_options_];
  }
}

bool NativeWindowMac::IsKiosk() {
  return is_kiosk_;
}

void NativeWindowMac::SetBackgroundColor(SkColor color) {
  NOTIMPLEMENTED() << "TODO";
  /*
  base::ScopedCFTypeRef<CGColorRef> cgcolor(
      skia::CGColorCreateFromSkColor(color));
  // views::Widget adds a layer for the content view.
  auto* bridge = views::NativeWidgetMac::GetBridgeForNativeWindow(window_);
  NSView* compositor_superview =
      static_cast<ui::AcceleratedWidgetMacNSView*>(bridge)
          ->AcceleratedWidgetGetNSView();
  [[compositor_superview layer] setBackgroundColor:cgcolor];
  // When using WebContents as content view, the contentView also has layer.
  if ([[window_ contentView] wantsLayer])
    [[[window_ contentView] layer] setBackgroundColor:cgcolor];
    */
}

void NativeWindowMac::SetHasShadow(bool has_shadow) {
  [window_ setHasShadow:has_shadow];
}

bool NativeWindowMac::HasShadow() {
  return [window_ hasShadow];
}

void NativeWindowMac::SetOpacity(const double opacity) {
  [window_ setAlphaValue:opacity];
}

double NativeWindowMac::GetOpacity() {
  return [window_ alphaValue];
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

void NativeWindowMac::SetIgnoreMouseEvents(bool ignore, bool forward) {
  [window_ setIgnoresMouseEvents:ignore];

  if (!ignore) {
    SetForwardMouseMessages(NO);
  } else {
    SetForwardMouseMessages(forward);
  }
}

void NativeWindowMac::SetContentProtection(bool enable) {
  [window_
      setSharingType:enable ? NSWindowSharingNone : NSWindowSharingReadOnly];
}

void NativeWindowMac::SetBrowserView(NativeBrowserView* view) {
  [CATransaction begin];
  [CATransaction setDisableActions:YES];

  if (browser_view()) {
    [browser_view()->GetInspectableWebContentsView()->GetNativeView()
            removeFromSuperview];
    set_browser_view(nullptr);
  }

  if (!view) {
    [CATransaction commit];
    return;
  }

  set_browser_view(view);
  auto* native_view = view->GetInspectableWebContentsView()->GetNativeView();
  [[window_ contentView] addSubview:native_view
                         positioned:NSWindowAbove
                         relativeTo:nil];
  native_view.hidden = NO;

  [CATransaction commit];
}

void NativeWindowMac::SetParentWindow(NativeWindow* parent) {
  InternalSetParentWindow(parent, IsVisible());
}

gfx::NativeView NativeWindowMac::GetNativeView() const {
  return [window_ contentView];
}

gfx::NativeWindow NativeWindowMac::GetNativeWindow() const {
  return window_;
}

gfx::AcceleratedWidget NativeWindowMac::GetAcceleratedWidget() const {
  return gfx::kNullAcceleratedWidget;
}

void NativeWindowMac::SetProgressBar(double progress,
                                     const NativeWindow::ProgressState state) {
  NSDockTile* dock_tile = [NSApp dockTile];

  // For the first time API invoked, we need to create a ContentView in
  // DockTile.
  if (dock_tile.contentView == nullptr) {
    NSImageView* image_view = [[NSImageView alloc] init];
    [image_view setImage:[NSApp applicationIconImage]];
    [dock_tile setContentView:image_view];
  }

  if ([[dock_tile.contentView subviews] count] == 0) {
    NSProgressIndicator* progress_indicator = [[AtomProgressBar alloc]
        initWithFrame:NSMakeRect(0.0f, 0.0f, dock_tile.size.width, 15.0)];
    [progress_indicator setStyle:NSProgressIndicatorBarStyle];
    [progress_indicator setIndeterminate:NO];
    [progress_indicator setBezeled:YES];
    [progress_indicator setMinValue:0];
    [progress_indicator setMaxValue:1];
    [progress_indicator setHidden:NO];
    [dock_tile.contentView addSubview:progress_indicator];
  }

  NSProgressIndicator* progress_indicator = static_cast<NSProgressIndicator*>(
      [[[dock_tile contentView] subviews] objectAtIndex:0]);
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
                                     const std::string& description) {}

void NativeWindowMac::SetVisibleOnAllWorkspaces(bool visible,
                                                bool visibleOnFullScreen) {
  SetCollectionBehavior(visible, NSWindowCollectionBehaviorCanJoinAllSpaces);
  SetCollectionBehavior(visibleOnFullScreen,
                        NSWindowCollectionBehaviorFullScreenAuxiliary);
}

bool NativeWindowMac::IsVisibleOnAllWorkspaces() {
  NSUInteger collectionBehavior = [window_ collectionBehavior];
  return collectionBehavior & NSWindowCollectionBehaviorCanJoinAllSpaces;
}

void NativeWindowMac::SetAutoHideCursor(bool auto_hide) {
  [window_ setDisableAutoHideCursor:!auto_hide];
}

void NativeWindowMac::SelectPreviousTab() {
  if (@available(macOS 10.12, *)) {
    [window_ selectPreviousTab:nil];
  }
}

void NativeWindowMac::SelectNextTab() {
  if (@available(macOS 10.12, *)) {
    [window_ selectNextTab:nil];
  }
}

void NativeWindowMac::MergeAllWindows() {
  if (@available(macOS 10.12, *)) {
    [window_ mergeAllWindows:nil];
  }
}

void NativeWindowMac::MoveTabToNewWindow() {
  if (@available(macOS 10.12, *)) {
    [window_ moveTabToNewWindow:nil];
  }
}

void NativeWindowMac::ToggleTabBar() {
  if (@available(macOS 10.12, *)) {
    [window_ toggleTabBar:nil];
  }
}

bool NativeWindowMac::AddTabbedWindow(NativeWindow* window) {
  if (window_ == window->GetNativeWindow()) {
    return false;
  } else {
    if (@available(macOS 10.12, *))
      [window_ addTabbedWindow:window->GetNativeWindow() ordered:NSWindowAbove];
  }
  return true;
}

bool NativeWindowMac::SetWindowButtonVisibility(bool visible) {
  if (title_bar_style_ == CUSTOM_BUTTONS_ON_HOVER) {
    return false;
  }

  window_button_visibility_ = visible;

  [[window_ standardWindowButton:NSWindowCloseButton] setHidden:!visible];
  [[window_ standardWindowButton:NSWindowMiniaturizeButton] setHidden:!visible];
  [[window_ standardWindowButton:NSWindowZoomButton] setHidden:!visible];
  return true;
}

void NativeWindowMac::SetVibrancy(const std::string& type) {
  if (@available(macOS 10.10, *)) {
    NSView* vibrant_view = [window_ vibrantView];

    if (type.empty()) {
      if (background_color_before_vibrancy_) {
        [window_ setBackgroundColor:background_color_before_vibrancy_];
        [window_ setTitlebarAppearsTransparent:transparency_before_vibrancy_];
      }
      if (vibrant_view == nil)
        return;

      [vibrant_view removeFromSuperview];
      [window_ setVibrantView:nil];
      ui::GpuSwitchingManager::SetTransparent(transparent());

      return;
    }

    background_color_before_vibrancy_.reset([[window_ backgroundColor] retain]);
    transparency_before_vibrancy_ = [window_ titlebarAppearsTransparent];
    ui::GpuSwitchingManager::SetTransparent(true);

    if (title_bar_style_ != NORMAL) {
      [window_ setTitlebarAppearsTransparent:YES];
      [window_ setBackgroundColor:[NSColor clearColor]];
    }

    NSVisualEffectView* effect_view = (NSVisualEffectView*)vibrant_view;
    if (effect_view == nil) {
      effect_view = [[[NSVisualEffectView alloc]
          initWithFrame:[[window_ contentView] bounds]] autorelease];
      [window_ setVibrantView:(NSView*)effect_view];

      [effect_view
          setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
      [effect_view setBlendingMode:NSVisualEffectBlendingModeBehindWindow];
      [effect_view setState:NSVisualEffectStateActive];
      [[window_ contentView] addSubview:effect_view
                             positioned:NSWindowBelow
                             relativeTo:nil];
    }

    NSVisualEffectMaterial vibrancyType = NSVisualEffectMaterialLight;

    if (type == "appearance-based") {
      vibrancyType = NSVisualEffectMaterialAppearanceBased;
    } else if (type == "light") {
      vibrancyType = NSVisualEffectMaterialLight;
    } else if (type == "dark") {
      vibrancyType = NSVisualEffectMaterialDark;
    } else if (type == "titlebar") {
      vibrancyType = NSVisualEffectMaterialTitlebar;
    }

    if (@available(macOS 10.11, *)) {
      // TODO(kevinsawicki): Use NSVisualEffectMaterial* constants directly once
      // they are available in the minimum SDK version
      if (type == "selection") {
        // NSVisualEffectMaterialSelection
        vibrancyType = static_cast<NSVisualEffectMaterial>(4);
      } else if (type == "menu") {
        // NSVisualEffectMaterialMenu
        vibrancyType = static_cast<NSVisualEffectMaterial>(5);
      } else if (type == "popover") {
        // NSVisualEffectMaterialPopover
        vibrancyType = static_cast<NSVisualEffectMaterial>(6);
      } else if (type == "sidebar") {
        // NSVisualEffectMaterialSidebar
        vibrancyType = static_cast<NSVisualEffectMaterial>(7);
      } else if (type == "medium-light") {
        // NSVisualEffectMaterialMediumLight
        vibrancyType = static_cast<NSVisualEffectMaterial>(8);
      } else if (type == "ultra-dark") {
        // NSVisualEffectMaterialUltraDark
        vibrancyType = static_cast<NSVisualEffectMaterial>(9);
      }
    }

    [effect_view setMaterial:vibrancyType];
  }
}

void NativeWindowMac::SetTouchBar(
    const std::vector<mate::PersistentDictionary>& items) {
  if (@available(macOS 10.12.2, *)) {
    touch_bar_.reset([[AtomTouchBar alloc]
        initWithDelegate:window_delegate_.get()
                  window:this
                settings:items]);
    [window_ setTouchBar:nil];
  }
}

void NativeWindowMac::RefreshTouchBarItem(const std::string& item_id) {
  if (@available(macOS 10.12.2, *)) {
    if (touch_bar_ && [window_ touchBar])
      [touch_bar_ refreshTouchBarItem:[window_ touchBar] id:item_id];
  }
}

void NativeWindowMac::SetEscapeTouchBarItem(
    const mate::PersistentDictionary& item) {
  if (@available(macOS 10.12.2, *)) {
    if (touch_bar_ && [window_ touchBar])
      [touch_bar_ setEscapeTouchBarItem:item forTouchBar:[window_ touchBar]];
  }
}

gfx::Rect NativeWindowMac::ContentBoundsToWindowBounds(
    const gfx::Rect& bounds) const {
  if (has_frame()) {
    gfx::Rect window_bounds(
        [window_ frameRectForContentRect:bounds.ToCGRect()]);
    int frame_height = window_bounds.height() - bounds.height();
    window_bounds.set_y(window_bounds.y() - frame_height);
    return window_bounds;
  } else {
    return bounds;
  }
}

gfx::Rect NativeWindowMac::WindowBoundsToContentBounds(
    const gfx::Rect& bounds) const {
  if (has_frame()) {
    gfx::Rect content_bounds(
        [window_ contentRectForFrameRect:bounds.ToCGRect()]);
    int frame_height = bounds.height() - content_bounds.height();
    content_bounds.set_y(content_bounds.y() + frame_height);
    return content_bounds;
  } else {
    return bounds;
  }
}

bool NativeWindowMac::CanResize() const {
  return resizable_;
}

views::View* NativeWindowMac::GetContentsView() {
  return root_view_.get();
}

void NativeWindowMac::AddContentViewLayers() {
  // Make sure the bottom corner is rounded for non-modal windows:
  // http://crbug.com/396264. But do not enable it on OS X 10.9 for transparent
  // window, otherwise a semi-transparent frame would show.
  if (!(transparent() && base::mac::IsOS10_9()) && !is_modal()) {
    // For normal window, we need to explicitly set layer for contentView to
    // make setBackgroundColor work correctly.
    // There is no need to do so for frameless window, and doing so would make
    // titleBarStyle stop working.
    if (has_frame()) {
      base::scoped_nsobject<CALayer> background_layer([[CALayer alloc] init]);
      [background_layer
          setAutoresizingMask:kCALayerWidthSizable | kCALayerHeightSizable];
      [[window_ contentView] setLayer:background_layer];
    }
    [[window_ contentView] setWantsLayer:YES];
  }

  if (!has_frame()) {
    // In OSX 10.10, adding subviews to the root view for the NSView hierarchy
    // produces warnings. To eliminate the warnings, we resize the contentView
    // to fill the window, and add subviews to that.
    // http://crbug.com/380412
    if (!original_set_frame_size) {
      Class cl = [[window_ contentView] class];
      original_set_frame_size = class_replaceMethod(
          cl, @selector(setFrameSize:), (IMP)SetFrameSize, "v@:{_NSSize=ff}");
      original_view_did_move_to_superview =
          class_replaceMethod(cl, @selector(viewDidMoveToSuperview),
                              (IMP)ViewDidMoveToSuperview, "v@:");
      [[window_ contentView] viewDidMoveToWindow];
    }

    // The fullscreen button should always be hidden for frameless window.
    [[window_ standardWindowButton:NSWindowFullScreenButton] setHidden:YES];

    if (title_bar_style_ == CUSTOM_BUTTONS_ON_HOVER) {
      buttons_view_.reset(
          [[CustomWindowButtonView alloc] initWithFrame:NSZeroRect]);
      [[window_ contentView] addSubview:buttons_view_];
    } else {
      if (title_bar_style_ != NORMAL) {
        if (base::mac::IsOS10_9()) {
          ShowWindowButton(NSWindowZoomButton);
          ShowWindowButton(NSWindowMiniaturizeButton);
          ShowWindowButton(NSWindowCloseButton);
        }
        return;
      }

      // Hide the window buttons.
      [[window_ standardWindowButton:NSWindowZoomButton] setHidden:YES];
      [[window_ standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
      [[window_ standardWindowButton:NSWindowCloseButton] setHidden:YES];
    }

    // Some third-party macOS utilities check the zoom button's enabled state to
    // determine whether to show custom UI on hover, so we disable it here to
    // prevent them from doing so in a frameless app window.
    [[window_ standardWindowButton:NSWindowZoomButton] setEnabled:NO];
  }
}

void NativeWindowMac::InternalSetParentWindow(NativeWindow* parent,
                                              bool attach) {
  if (is_modal())
    return;

  NativeWindow::SetParentWindow(parent);

  // Do not remove/add if we are already properly attached.
  if (attach && parent && [window_ parentWindow] == parent->GetNativeWindow())
    return;

  // Remove current parent window.
  if ([window_ parentWindow])
    [[window_ parentWindow] removeChildWindow:window_];

  // Set new parent window.
  // Note that this method will force the window to become visible.
  if (parent && attach)
    [parent->GetNativeWindow() addChildWindow:window_ ordered:NSWindowAbove];
}

void NativeWindowMac::ShowWindowButton(NSWindowButton button) {
  auto view = [window_ standardWindowButton:button];
  [view.superview addSubview:view positioned:NSWindowAbove relativeTo:nil];
}

void NativeWindowMac::SetForwardMouseMessages(bool forward) {
  [window_ setAcceptsMouseMovedEvents:forward];
}

void NativeWindowMac::OverrideNSWindowContentView() {
  // When using `views::Widget` to hold WebContents, Chromium would use
  // `BridgedContentView` as content view, which does not support draggable
  // regions. In order to make draggable regions work, we have to replace the
  // content view with a simple NSView.
  container_view_.reset([[FullSizeContentView alloc] init]);
  [container_view_
      setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  [container_view_ setFrame:[[[window_ contentView] superview] bounds]];
  [window_ setContentView:container_view_];
  AddContentViewLayers();
}

void NativeWindowMac::SetStyleMask(bool on, NSUInteger flag) {
  // Changing the styleMask of a frameless windows causes it to change size so
  // we explicitly disable resizing while setting it.
  ScopedDisableResize disable_resize;

  bool was_maximizable = IsMaximizable();
  if (on)
    [window_ setStyleMask:[window_ styleMask] | flag];
  else
    [window_ setStyleMask:[window_ styleMask] & (~flag)];
  // Change style mask will make the zoom button revert to default, probably
  // a bug of Cocoa or macOS.
  SetMaximizable(was_maximizable);
}

void NativeWindowMac::SetCollectionBehavior(bool on, NSUInteger flag) {
  bool was_maximizable = IsMaximizable();
  if (on)
    [window_ setCollectionBehavior:[window_ collectionBehavior] | flag];
  else
    [window_ setCollectionBehavior:[window_ collectionBehavior] & (~flag)];
  // Change collectionBehavior will make the zoom button revert to default,
  // probably a bug of Cocoa or macOS.
  SetMaximizable(was_maximizable);
}

// static
NativeWindow* NativeWindow::Create(const mate::Dictionary& options,
                                   NativeWindow* parent) {
  return new NativeWindowMac(options, parent);
}

}  // namespace atom
