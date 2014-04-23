// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/native_window_mac.h"

#include <string>

#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#import "atom/browser/ui/cocoa/event_processing_window.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "atom/common/draggable_region.h"
#include "atom/common/options_switches.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/browser/render_view_host.h"

static const CGFloat kAtomWindowCornerRadius = 4.0;

@interface NSView (PrivateMethods)
- (CGFloat)roundedCornerRadius;
@end

@interface AtomNSWindowDelegate : NSObject<NSWindowDelegate> {
 @private
  atom::NativeWindowMac* shell_;
  BOOL acceptsFirstMouse_;
}
- (id)initWithShell:(atom::NativeWindowMac*)shell;
- (void)setAcceptsFirstMouse:(BOOL)accept;
@end

@implementation AtomNSWindowDelegate

- (id)initWithShell:(atom::NativeWindowMac*)shell {
  if ((self = [super init])) {
    shell_ = shell;
    acceptsFirstMouse_ = NO;
  }
  return self;
}

- (void)setAcceptsFirstMouse:(BOOL)accept {
  acceptsFirstMouse_ = accept;
}

- (void)windowDidResignMain:(NSNotification*)notification {
  shell_->NotifyWindowBlur();
}

- (void)windowDidResize:(NSNotification*)otification {
  if (!shell_->has_frame())
    shell_->ClipWebView();
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
  if (!shell_->has_frame()) {
    NSWindow* window = shell_->GetNativeWindow();
    [[window standardWindowButton:NSWindowFullScreenButton] setHidden:YES];
  }
}

- (void)windowWillClose:(NSNotification*)notification {
  shell_->NotifyWindowClosed();
  [self autorelease];
}

- (BOOL)windowShouldClose:(id)window {
  // When user tries to close the window by clicking the close button, we do
  // not close the window immediately, instead we try to close the web page
  // fisrt, and when the web page is closed the window will also be closed.
  shell_->CloseWebContents();
  return NO;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
  return acceptsFirstMouse_;
}

@end

@interface AtomNSWindow : EventProcessingWindow {
 @protected
  atom::NativeWindowMac* shell_;
}
- (void)setShell:(atom::NativeWindowMac*)shell;
@end

@implementation AtomNSWindow

- (void)setShell:(atom::NativeWindowMac*)shell {
  shell_ = shell;
}

- (IBAction)reload:(id)sender {
  shell_->GetWebContents()->GetController().ReloadIgnoringCache(false);
}

- (IBAction)showDevTools:(id)sender {
  shell_->OpenDevTools();
}

@end

@interface ControlRegionView : NSView {
 @private
  atom::NativeWindowMac* shellWindow_;  // Weak; owns self.
}
@end

@implementation ControlRegionView

- (id)initWithShellWindow:(atom::NativeWindowMac*)shellWindow {
  if ((self = [super init]))
    shellWindow_ = shellWindow;
  return self;
}

- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

- (NSView*)hitTest:(NSPoint)aPoint {
  if (!shellWindow_->IsWithinDraggableRegion(aPoint)) {
    return nil;
  }
  return self;
}

- (void)mouseDown:(NSEvent*)event {
  shellWindow_->HandleMouseEvent(event);
}

- (void)mouseDragged:(NSEvent*)event {
  shellWindow_->HandleMouseEvent(event);
}

@end

namespace atom {

NativeWindowMac::NativeWindowMac(content::WebContents* web_contents,
                                 base::DictionaryValue* options)
    : NativeWindow(web_contents, options),
      is_kiosk_(false),
      attention_request_id_(0) {
  int width = 800, height = 600;
  options->GetInteger(switches::kWidth, &width);
  options->GetInteger(switches::kHeight, &height);

  NSRect main_screen_rect = [[[NSScreen screens] objectAtIndex:0] frame];
  NSRect cocoa_bounds = NSMakeRect(
      round((NSWidth(main_screen_rect) - width) / 2) ,
      round((NSHeight(main_screen_rect) - height) / 2),
      width,
      height);

  AtomNSWindow* atomWindow;

  atomWindow = [[AtomNSWindow alloc]
      initWithContentRect:cocoa_bounds
                styleMask:NSTitledWindowMask | NSClosableWindowMask |
                          NSMiniaturizableWindowMask | NSResizableWindowMask |
                          NSTexturedBackgroundWindowMask
                  backing:NSBackingStoreBuffered
                    defer:YES];

  [atomWindow setShell:this];
  window_.reset(atomWindow);

  AtomNSWindowDelegate* delegate =
      [[AtomNSWindowDelegate alloc] initWithShell:this];
  [window_ setDelegate:delegate];

  // We will manage window's lifetime ourselves.
  [window_ setReleasedWhenClosed:NO];

  // Enable the NSView to accept first mouse event.
  bool acceptsFirstMouse = false;
  options->GetBoolean(switches::kAcceptFirstMouse, &acceptsFirstMouse);
  [delegate setAcceptsFirstMouse:acceptsFirstMouse];

  // Disable fullscreen button when 'fullscreen' is specified to false.
  bool fullscreen;
  if (!(options->GetBoolean(switches::kFullscreen, &fullscreen) &&
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
  // Force InspectableWebContents to be destroyed before we destroy window,
  // because it may still be observing the window at this time.
  DestroyWebContents();
}

void NativeWindowMac::Close() {
  [window_ performClose:nil];
}

void NativeWindowMac::CloseImmediately() {
  [window_ orderOut:nil];
  [window_ close];
}

void NativeWindowMac::Move(const gfx::Rect& pos) {
  NSRect cocoa_bounds = NSMakeRect(pos.x(), 0,
                                   pos.width(),
                                   pos.height());
  // Flip coordinates based on the primary screen.
  NSScreen* screen = [[NSScreen screens] objectAtIndex:0];
  cocoa_bounds.origin.y =
      NSHeight([screen frame]) - pos.height() - pos.y();

  [window_ setFrame:cocoa_bounds display:YES];
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
  [window_ makeKeyAndOrderFront:nil];
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

void NativeWindowMac::Minimize() {
  [window_ miniaturize:nil];
}

void NativeWindowMac::Restore() {
  [window_ deminiaturize:nil];
}

void NativeWindowMac::SetFullscreen(bool fullscreen) {
  if (fullscreen == IsFullscreen())
    return;

  if (!base::mac::IsOSLionOrLater()) {
    LOG(ERROR) << "Fullscreen mode is only supported above Lion";
    return;
  }

  [window_ toggleFullScreen:nil];
}

bool NativeWindowMac::IsFullscreen() {
  return [window_ styleMask] & NSFullScreenWindowMask;
}

void NativeWindowMac::SetSize(const gfx::Size& size) {
  NSRect frame = [window_ frame];
  frame.origin.y -= size.height() - frame.size.height;
  frame.size.width = size.width();
  frame.size.height = size.height();

  [window_ setFrame:frame display:YES];
}

gfx::Size NativeWindowMac::GetSize() {
  NSRect frame = [window_ frame];
  return gfx::Size(frame.size.width, frame.size.height);
}

void NativeWindowMac::SetMinimumSize(const gfx::Size& size) {
  NSSize min_size = NSMakeSize(size.width(), size.height());
  NSView* content = [window_ contentView];
  [window_ setContentMinSize:[content convertSize:min_size toView:nil]];
}

gfx::Size NativeWindowMac::GetMinimumSize() {
  NSView* content = [window_ contentView];
  NSSize min_size = [content convertSize:[window_ contentMinSize]
                                fromView:nil];
  return gfx::Size(min_size.width, min_size.height);
}

void NativeWindowMac::SetMaximumSize(const gfx::Size& size) {
  NSSize max_size = NSMakeSize(size.width(), size.height());
  NSView* content = [window_ contentView];
  [window_ setContentMaxSize:[content convertSize:max_size toView:nil]];
}

gfx::Size NativeWindowMac::GetMaximumSize() {
  NSView* content = [window_ contentView];
  NSSize max_size = [content convertSize:[window_ contentMaxSize]
                                fromView:nil];
  return gfx::Size(max_size.width, max_size.height);
}

void NativeWindowMac::SetResizable(bool resizable) {
  if (resizable) {
    [[window_ standardWindowButton:NSWindowZoomButton] setEnabled:YES];
    [window_ setStyleMask:[window_ styleMask] | NSResizableWindowMask];
  } else {
    [[window_ standardWindowButton:NSWindowZoomButton] setEnabled:NO];
    [window_ setStyleMask:[window_ styleMask] ^ NSResizableWindowMask];
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

void NativeWindowMac::SetPosition(const gfx::Point& position) {
  Move(gfx::Rect(position, GetSize()));
}

gfx::Point NativeWindowMac::GetPosition() {
  NSRect frame = [window_ frame];
  NSScreen* screen = [[NSScreen screens] objectAtIndex:0];

  return gfx::Point(frame.origin.x,
      NSHeight([screen frame]) - frame.origin.y - frame.size.height);
}

void NativeWindowMac::SetTitle(const std::string& title) {
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

void NativeWindowMac::SetKiosk(bool kiosk) {
  if (kiosk) {
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
    SetFullscreen(true);
  } else {
    [NSApp setPresentationOptions:[NSApp currentSystemPresentationOptions]];
    is_kiosk_  = false;
    SetFullscreen(false);
  }
}

bool NativeWindowMac::IsKiosk() {
  return is_kiosk_;
}

bool NativeWindowMac::HasModalDialog() {
  return [window_ attachedSheet] != nil;
}

gfx::NativeWindow NativeWindowMac::GetNativeWindow() {
  return window_;
}

bool NativeWindowMac::IsWithinDraggableRegion(NSPoint point) const {
  if (!draggable_region_)
    return false;
  NSView* webView = GetWebContents()->GetView()->GetNativeView();
  NSInteger webViewHeight = NSHeight([webView bounds]);
  // |draggable_region_| is stored in local platform-indepdent coordiate system
  // while |point| is in local Cocoa coordinate system. Do the conversion
  // to match these two.
  return draggable_region_->contains(point.x, webViewHeight - point.y);
}

void NativeWindowMac::HandleMouseEvent(NSEvent* event) {
  NSPoint current_mouse_location =
      [window_ convertBaseToScreen:[event locationInWindow]];

  if ([event type] == NSLeftMouseDown) {
    NSPoint frame_origin = [window_ frame].origin;
    last_mouse_offset_ = NSMakePoint(
        frame_origin.x - current_mouse_location.x,
        frame_origin.y - current_mouse_location.y);
  } else if ([event type] == NSLeftMouseDragged) {
    [window_ setFrameOrigin:NSMakePoint(
        current_mouse_location.x + last_mouse_offset_.x,
        current_mouse_location.y + last_mouse_offset_.y)];
  }
}

void NativeWindowMac::UpdateDraggableRegions(
    const std::vector<DraggableRegion>& regions) {
  // Draggable region is not supported for non-frameless window.
  if (has_frame_)
    return;

  UpdateDraggableRegionsForCustomDrag(regions);
  InstallDraggableRegionViews();
}

void NativeWindowMac::HandleKeyboardEvent(
    content::WebContents*,
    const content::NativeWebKeyboardEvent& event) {
  if (event.skip_in_browser ||
      event.type == content::NativeWebKeyboardEvent::Char)
    return;

  EventProcessingWindow* event_window =
      static_cast<EventProcessingWindow*>(window_);
  DCHECK([event_window isKindOfClass:[EventProcessingWindow class]]);
  [event_window redispatchKeyEvent:event.os_event];
}

void NativeWindowMac::InstallView() {
  NSView* view = inspectable_web_contents()->GetView()->GetNativeView();
  if (has_frame_) {
    [view setFrame:[[window_ contentView] bounds]];
    [[window_ contentView] addSubview:view];
  } else {
    NSView* frameView = [[window_ contentView] superview];
    [view setFrame:[frameView bounds]];
    [frameView addSubview:view];

    ClipWebView();

    [[window_ standardWindowButton:NSWindowZoomButton] setHidden:YES];
    [[window_ standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
    [[window_ standardWindowButton:NSWindowCloseButton] setHidden:YES];
    [[window_ standardWindowButton:NSWindowFullScreenButton] setHidden:YES];
  }
}

void NativeWindowMac::UninstallView() {
  NSView* view = inspectable_web_contents()->GetView()->GetNativeView();
  [view removeFromSuperview];
}

void NativeWindowMac::ClipWebView() {
  NSView* view = GetWebContents()->GetView()->GetNativeView();

  view.wantsLayer = YES;
  view.layer.masksToBounds = YES;
  view.layer.cornerRadius = kAtomWindowCornerRadius;
}

void NativeWindowMac::InstallDraggableRegionViews() {
  DCHECK(!has_frame_);

  // All ControlRegionViews should be added as children of the WebContentsView,
  // because WebContentsView will be removed and re-added when entering and
  // leaving fullscreen mode.
  NSView* webView = GetWebContents()->GetView()->GetNativeView();
  NSInteger webViewHeight = NSHeight([webView bounds]);

  // Remove all ControlRegionViews that are added last time.
  // Note that [webView subviews] returns the view's mutable internal array and
  // it should be copied to avoid mutating the original array while enumerating
  // it.
  base::scoped_nsobject<NSArray> subviews([[webView subviews] copy]);
  for (NSView* subview in subviews.get())
    if ([subview isKindOfClass:[ControlRegionView class]])
      [subview removeFromSuperview];

  // Create and add ControlRegionView for each region that needs to be excluded
  // from the dragging.
  for (std::vector<gfx::Rect>::const_iterator iter =
           system_drag_exclude_areas_.begin();
       iter != system_drag_exclude_areas_.end();
       ++iter) {
    base::scoped_nsobject<NSView> controlRegion(
        [[ControlRegionView alloc] initWithShellWindow:this]);
    [controlRegion setFrame:NSMakeRect(iter->x(),
                                       webViewHeight - iter->bottom(),
                                       iter->width(),
                                       iter->height())];
    [webView addSubview:controlRegion];
  }
}

void NativeWindowMac::UpdateDraggableRegionsForCustomDrag(
    const std::vector<DraggableRegion>& regions) {
  // We still need one ControlRegionView to cover the whole window such that
  // mouse events could be captured.
  NSView* web_view = GetWebContents()->GetView()->GetNativeView();
  gfx::Rect window_bounds(
      0, 0, NSWidth([web_view bounds]), NSHeight([web_view bounds]));
  system_drag_exclude_areas_.clear();
  system_drag_exclude_areas_.push_back(window_bounds);

  // Aggregate the draggable areas and non-draggable areas such that hit test
  // could be performed easily.
  SkRegion* draggable_region = new SkRegion;
  for (std::vector<DraggableRegion>::const_iterator iter = regions.begin();
       iter != regions.end();
       ++iter) {
    const DraggableRegion& region = *iter;
    draggable_region->op(
        region.bounds.x(),
        region.bounds.y(),
        region.bounds.right(),
        region.bounds.bottom(),
        region.draggable ? SkRegion::kUnion_Op : SkRegion::kDifference_Op);
  }
  draggable_region_.reset(draggable_region);
}

// static
NativeWindow* NativeWindow::Create(content::WebContents* web_contents,
                                   base::DictionaryValue* options) {
  return new NativeWindowMac(web_contents, options);
}

}  // namespace atom
