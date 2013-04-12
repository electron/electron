// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/native_window_mac.h"

// FIXME: The foundation_util.h is aborting our compilation, do not
// include it.
#define BASE_MAC_FOUNDATION_UTIL_H_

#include "base/mac/mac_util.h"
#include "base/sys_string_conversions.h"
#include "base/values.h"
#import "browser/atom_event_processing_window.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "common/options_switches.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"

@interface AtomNSWindow : AtomEventProcessingWindow {
 @private
  atom::NativeWindowMac* shell_;
}
- (void)setShell:(atom::NativeWindowMac*)shell;
- (IBAction)showDevTools:(id)sender;
@end

@implementation AtomNSWindow

- (void)setShell:(atom::NativeWindowMac*)shell {
  shell_ = shell;
}

- (IBAction)showDevTools:(id)sender {
  shell_->ShowDevTools();
}

@end

namespace atom {

NativeWindowMac::NativeWindowMac(content::BrowserContext* browser_context,
                                 base::DictionaryValue* options)
    : NativeWindow(browser_context, options),
      is_fullscreen_(false),
      is_kiosk_(false),
      attention_request_id_(0) {
  int width, height;
  options->GetInteger(switches::kWidth, &width);
  options->GetInteger(switches::kHeight, &height);

  NSRect main_screen_rect = [[[NSScreen screens] objectAtIndex:0] frame];
  NSRect cocoa_bounds = NSMakeRect(
      (NSWidth(main_screen_rect) - width) / 2,
      (NSHeight(main_screen_rect) - height) / 2,
      width,
      height);
  NSUInteger style_mask = NSTitledWindowMask | NSClosableWindowMask |
                          NSMiniaturizableWindowMask | NSResizableWindowMask |
                          NSTexturedBackgroundWindowMask;
  AtomNSWindow* atom_window = [[AtomNSWindow alloc]
      initWithContentRect:cocoa_bounds
                styleMask:style_mask
                  backing:NSBackingStoreBuffered
                    defer:YES];
  [atom_window setShell:this];

  window_ = atom_window;

  // Disable fullscreen button when 'fullscreen' is specified to false.
  bool fullscreen;
  if (!(options->GetBoolean(switches::kFullscreen, &fullscreen) &&
        !fullscreen)) {
    NSUInteger collectionBehavior = [window() collectionBehavior];
    collectionBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;
    [window() setCollectionBehavior:collectionBehavior];
  }

  InstallView();
}

NativeWindowMac::~NativeWindowMac() {
}

void NativeWindowMac::Close() {
  [window() performClose:nil];
}

void NativeWindowMac::Move(const gfx::Rect& pos) {
  NSRect cocoa_bounds = NSMakeRect(pos.x(), 0,
                                   pos.width(),
                                   pos.height());
  // Flip coordinates based on the primary screen.
  NSScreen* screen = [[NSScreen screens] objectAtIndex:0];
  cocoa_bounds.origin.y =
      NSHeight([screen frame]) - pos.height() - pos.y();

  [window() setFrame:cocoa_bounds display:YES];
}

void NativeWindowMac::Focus(bool focus) {
  if (focus && [window() isVisible])
    [window() makeKeyAndOrderFront:nil];
  else
    [window() orderBack:nil];
}

void NativeWindowMac::Show() {
  [window() makeKeyAndOrderFront:nil];
}

void NativeWindowMac::Hide() {
  [window() orderOut:nil];
}

void NativeWindowMac::Maximize() {
  [window() zoom:nil];
}

void NativeWindowMac::Unmaximize() {
  [window() zoom:nil];
}

void NativeWindowMac::Minimize() {
  [window() miniaturize:nil];
}

void NativeWindowMac::Restore() {
  [window() deminiaturize:nil];
}

void NativeWindowMac::SetFullscreen(bool fullscreen) {
  if (fullscreen == is_fullscreen_)
    return;

  if (base::mac::IsOSLionOrLater()) {
    is_fullscreen_ = fullscreen;
    [window() toggleFullScreen:nil];
    return;
  }

  DCHECK(base::mac::IsOSSnowLeopard());

  SetNonLionFullscreen(fullscreen);
}

bool NativeWindowMac::IsFullscreen() {
  return is_fullscreen_;
}

void NativeWindowMac::SetNonLionFullscreen(bool fullscreen) {
  if (fullscreen == is_fullscreen_)
    return;

  is_fullscreen_ = fullscreen;

  // Fade to black.
  const CGDisplayReservationInterval kFadeDurationSeconds = 0.6;
  bool did_fade_out = false;
  CGDisplayFadeReservationToken token;
  if (CGAcquireDisplayFadeReservation(kFadeDurationSeconds, &token) ==
      kCGErrorSuccess) {
    did_fade_out = true;
    CGDisplayFade(token, kFadeDurationSeconds / 2, kCGDisplayBlendNormal,
        kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, /*synchronous=*/true);
  }

  // Since frameless windows insert the WebContentsView into the NSThemeFrame
  // ([[window contentView] superview]), and since that NSThemeFrame is
  // destroyed and recreated when we change the styleMask of the window, we
  // need to remove the view from the window when we change the style, and
  // add it back afterwards.
  UninstallView();
  if (fullscreen) {
    restored_bounds_ = [window() frame];
    [window() setStyleMask:NSBorderlessWindowMask];
    [window() setFrame:[window()
        frameRectForContentRect:[[window() screen] frame]]
               display:YES];
    base::mac::RequestFullScreen(base::mac::kFullScreenModeAutoHideAll);
  } else {
    base::mac::ReleaseFullScreen(base::mac::kFullScreenModeAutoHideAll);
    NSUInteger style_mask = NSTitledWindowMask | NSClosableWindowMask |
                            NSMiniaturizableWindowMask | NSResizableWindowMask |
                            NSTexturedBackgroundWindowMask;
    [window() setStyleMask:style_mask];
    [window() setFrame:restored_bounds_ display:YES];
  }
  InstallView();

  // Fade back in.
  if (did_fade_out) {
    CGDisplayFade(token, kFadeDurationSeconds / 2, kCGDisplayBlendSolidColor,
        kCGDisplayBlendNormal, 0.0, 0.0, 0.0, /*synchronous=*/false);
    CGReleaseDisplayFadeReservation(token);
  }

  is_fullscreen_ = fullscreen;
}

void NativeWindowMac::SetSize(const gfx::Size& size) {
  NSRect frame = [window_ frame];
  frame.origin.y -= size.height() - frame.size.height;
  frame.size.width = size.width();
  frame.size.height = size.height();

  [window() setFrame:frame display:YES];
}

gfx::Size NativeWindowMac::GetSize() {
  NSRect frame = [window_ frame];
  return gfx::Size(frame.size.width, frame.size.height);
}

void NativeWindowMac::SetMinimumSize(int width, int height) {
  NSSize min_size = NSMakeSize(width, height);
  NSView* content = [window() contentView];
  [window() setContentMinSize:[content convertSize:min_size toView:nil]];
}

void NativeWindowMac::SetMaximumSize(int width, int height) {
  NSSize max_size = NSMakeSize(width, height);
  NSView* content = [window() contentView];
  [window() setContentMaxSize:[content convertSize:max_size toView:nil]];
}

void NativeWindowMac::SetResizable(bool resizable) {
  if (resizable) {
    [[window() standardWindowButton:NSWindowZoomButton] setEnabled:YES];
    [window() setStyleMask:window().styleMask | NSResizableWindowMask];
  } else {
    [[window() standardWindowButton:NSWindowZoomButton] setEnabled:NO];
    [window() setStyleMask:window().styleMask ^ NSResizableWindowMask];
  }
}

void NativeWindowMac::SetAlwaysOnTop(bool top) {
  [window() setLevel:(top ? NSFloatingWindowLevel : NSNormalWindowLevel)];
}

void NativeWindowMac::SetPosition(const std::string& position) {
  if (position == "center")
    [window() center];
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
  [window() setTitle:base::SysUTF8ToNSString(title)];
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
    SetNonLionFullscreen(true);
  } else {
    [NSApp setPresentationOptions:[NSApp currentSystemPresentationOptions]];
    is_kiosk_  = false;
    SetNonLionFullscreen(false);
  }
}

bool NativeWindowMac::IsKiosk() {
  return is_kiosk_;
}

void NativeWindowMac::HandleKeyboardEvent(
    content::WebContents*,
    const content::NativeWebKeyboardEvent& event) {
  if (event.skip_in_browser ||
      event.type == content::NativeWebKeyboardEvent::Char)
    return;

  AtomEventProcessingWindow* event_window =
      static_cast<AtomEventProcessingWindow*>(window());
  DCHECK([event_window isKindOfClass:[AtomEventProcessingWindow class]]);
  [event_window redispatchKeyEvent:event.os_event];
}

void NativeWindowMac::InstallView() {
  NSView* view = inspectable_web_contents()->GetView()->GetNativeView();
  [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  [view setFrame:[[window() contentView] bounds]];
  [[window() contentView] addSubview:view];
}

void NativeWindowMac::UninstallView() {
  NSView* view = GetWebContents()->GetView()->GetNativeView();
  [view removeFromSuperview];
}

// static
NativeWindow* NativeWindow::Create(content::BrowserContext* browser_context,
                                   base::DictionaryValue* options) {
  return new NativeWindowMac(browser_context, options);
}

}  // namespace atom
