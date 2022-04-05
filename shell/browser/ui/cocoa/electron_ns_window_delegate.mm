// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/electron_ns_window_delegate.h"

#include <algorithm>

#include "base/mac/mac_util.h"
#include "components/remote_cocoa/app_shim/native_widget_ns_window_bridge.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_window_mac.h"
#include "shell/browser/ui/cocoa/electron_preview_item.h"
#include "shell/browser/ui/cocoa/electron_touch_bar.h"
#include "ui/gfx/geometry/resize_utils.h"
#include "ui/gfx/mac/coordinate_conversion.h"
#include "ui/views/cocoa/native_widget_mac_ns_window_host.h"
#include "ui/views/widget/native_widget_mac.h"

using TitleBarStyle = electron::NativeWindowMac::TitleBarStyle;
using FullScreenTransitionState =
    electron::NativeWindowMac::FullScreenTransitionState;

@implementation ElectronNSWindowDelegate

- (id)initWithShell:(electron::NativeWindowMac*)shell {
  // The views library assumes the window delegate must be an instance of
  // ViewsNSWindowDelegate, since we don't have a way to override the creation
  // of NSWindowDelegate, we have to dynamically replace the window delegate
  // on the fly.
  // TODO(zcbenz): Add interface in NativeWidgetMac to allow overriding creating
  // window delegate.
  auto* bridge_host = views::NativeWidgetMacNSWindowHost::GetFromNativeWindow(
      shell->GetNativeWindow());
  auto* bridged_view = bridge_host->GetInProcessNSWindowBridge();
  if ((self = [super initWithBridgedNativeWidget:bridged_view])) {
    shell_ = shell;
    is_zooming_ = false;
    level_ = [shell_->GetNativeWindow().GetNativeNSWindow() level];
  }
  return self;
}

#pragma mark - NSWindowDelegate

- (void)windowDidChangeOcclusionState:(NSNotification*)notification {
  // notification.object is the window that changed its state.
  // It's safe to use self.window instead if you don't assign one delegate to
  // many windows
  NSWindow* window = notification.object;

  // check occlusion binary flag
  if (window.occlusionState & NSWindowOcclusionStateVisible) {
    // The app is visible
    shell_->NotifyWindowShow();
  } else {
    // The app is not visible
    shell_->NotifyWindowHide();
  }
}

// Called when the user clicks the zoom button or selects it from the Window
// menu to determine the "standard size" of the window.
- (NSRect)windowWillUseStandardFrame:(NSWindow*)window
                        defaultFrame:(NSRect)frame {
  if (!shell_->zoom_to_page_width()) {
    if (shell_->GetAspectRatio() > 0.0)
      shell_->set_default_frame_for_zoom(frame);
    return frame;
  }

  // If the shift key is down, maximize.
  if ([[NSApp currentEvent] modifierFlags] & NSShiftKeyMask)
    return frame;

  // Get preferred width from observers. Usually the page width.
  int preferred_width = 0;
  shell_->NotifyWindowRequestPreferredWidth(&preferred_width);

  // Never shrink from the current size on zoom.
  NSRect window_frame = [window frame];
  CGFloat zoomed_width =
      std::max(static_cast<CGFloat>(preferred_width), NSWidth(window_frame));

  // |frame| determines our maximum extents. We need to set the origin of the
  // frame -- and only move it left if necessary.
  if (window_frame.origin.x + zoomed_width > NSMaxX(frame))
    frame.origin.x = NSMaxX(frame) - zoomed_width;
  else
    frame.origin.x = window_frame.origin.x;

  // Set the width. Don't touch y or height.
  frame.size.width = zoomed_width;

  if (shell_->GetAspectRatio() > 0.0)
    shell_->set_default_frame_for_zoom(frame);

  return frame;
}

- (void)windowDidBecomeMain:(NSNotification*)notification {
  shell_->NotifyWindowFocus();
  shell_->RedrawTrafficLights();
}

- (void)windowDidResignMain:(NSNotification*)notification {
  shell_->NotifyWindowBlur();
  shell_->RedrawTrafficLights();
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
  shell_->NotifyWindowIsKeyChanged(true);
  shell_->RedrawTrafficLights();
}

- (void)windowDidResignKey:(NSNotification*)notification {
  // If our app is still active and we're still the key window, ignore this
  // message, since it just means that a menu extra (on the "system status bar")
  // was activated; we'll get another |-windowDidResignKey| if we ever really
  // lose key window status.
  if ([NSApp isActive] && ([NSApp keyWindow] == [notification object]))
    return;

  shell_->NotifyWindowIsKeyChanged(false);
  shell_->RedrawTrafficLights();
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

  if (!resizingHorizontally_) {
    NSWindow* window = shell_->GetNativeWindow().GetNativeNSWindow();
    const auto widthDelta = frameSize.width - [window frame].size.width;
    const auto heightDelta = frameSize.height - [window frame].size.height;
    resizingHorizontally_ = std::abs(widthDelta) > std::abs(heightDelta);
  }

  {
    bool prevent_default = false;
    NSRect new_bounds = NSMakeRect(sender.frame.origin.x, sender.frame.origin.y,
                                   newSize.width, newSize.height);
    shell_->NotifyWindowWillResize(gfx::ScreenRectFromNSRect(new_bounds),
                                   *resizingHorizontally_
                                       ? gfx::ResizeEdge::kRight
                                       : gfx::ResizeEdge::kBottom,
                                   &prevent_default);
    if (prevent_default) {
      return sender.frame.size;
    }
  }

  return newSize;
}

- (void)windowDidResize:(NSNotification*)notification {
  [super windowDidResize:notification];
  shell_->NotifyWindowResize();
  shell_->RedrawTrafficLights();
}

- (void)windowWillMove:(NSNotification*)notification {
  NSWindow* window = [notification object];
  NSSize size = [[window contentView] frame].size;
  NSRect new_bounds = NSMakeRect(window.frame.origin.x, window.frame.origin.y,
                                 size.width, size.height);
  bool prevent_default = false;

  // prevent_default has no effect
  shell_->NotifyWindowWillMove(gfx::ScreenRectFromNSRect(new_bounds),
                               &prevent_default);
}

- (void)windowDidMove:(NSNotification*)notification {
  [super windowDidMove:notification];
  // TODO(zcbenz): Remove the alias after figuring out a proper
  // way to dispatch move.
  shell_->NotifyWindowMove();
  shell_->NotifyWindowMoved();
}

- (void)windowWillMiniaturize:(NSNotification*)notification {
  NSWindow* window = shell_->GetNativeWindow().GetNativeNSWindow();
  // store the current status window level to be restored in
  // windowDidDeminiaturize
  level_ = [window level];
  shell_->SetWindowLevel(NSNormalWindowLevel);
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
  [super windowDidMiniaturize:notification];
  shell_->NotifyWindowMinimize();
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
  [super windowDidDeminiaturize:notification];
  shell_->SetWindowLevel(level_);
  shell_->NotifyWindowRestore();
}

- (BOOL)windowShouldZoom:(NSWindow*)window toFrame:(NSRect)newFrame {
  is_zooming_ = true;
  return YES;
}

- (void)windowDidEndLiveResize:(NSNotification*)notification {
  resizingHorizontally_.reset();
  shell_->NotifyWindowResized();
  if (is_zooming_) {
    if (shell_->IsMaximized())
      shell_->NotifyWindowMaximize();
    else
      shell_->NotifyWindowUnmaximize();
    is_zooming_ = false;
  }
}

- (void)windowWillEnterFullScreen:(NSNotification*)notification {
  shell_->SetFullScreenTransitionState(FullScreenTransitionState::ENTERING);

  shell_->NotifyWindowWillEnterFullScreen();

  // Setting resizable to true before entering fullscreen.
  is_resizable_ = shell_->IsResizable();
  shell_->SetResizable(true);
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
  shell_->SetFullScreenTransitionState(FullScreenTransitionState::NONE);

  shell_->NotifyWindowEnterFullScreen();

  shell_->HandlePendingFullscreenTransitions();
}

- (void)windowWillExitFullScreen:(NSNotification*)notification {
  shell_->SetFullScreenTransitionState(FullScreenTransitionState::EXITING);

  shell_->NotifyWindowWillLeaveFullScreen();
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
  shell_->SetFullScreenTransitionState(FullScreenTransitionState::NONE);

  shell_->SetResizable(is_resizable_);
  shell_->NotifyWindowLeaveFullScreen();

  shell_->HandlePendingFullscreenTransitions();
}

- (void)windowWillClose:(NSNotification*)notification {
  shell_->Cleanup();
  shell_->NotifyWindowClosed();

  // Something called -[NSWindow close] on a sheet rather than calling
  // -[NSWindow endSheet:] on its parent. If the modal session is not ended
  // then the parent will never be able to show another sheet. But calling
  // -endSheet: here will block the thread with an animation, so post a task.
  if (shell_->is_modal() && shell_->parent() && shell_->IsVisible()) {
    NSWindow* window = shell_->GetNativeWindow().GetNativeNSWindow();
    NSWindow* sheetParent = [window sheetParent];
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(base::RetainBlock(^{
          [sheetParent endSheet:window];
        })));
  }

  // Clears the delegate when window is going to be closed, since EL Capitan it
  // is possible that the methods of delegate would get called after the window
  // has been closed.
  auto* bridge_host = views::NativeWidgetMacNSWindowHost::GetFromNativeWindow(
      shell_->GetNativeWindow());
  auto* bridged_view = bridge_host->GetInProcessNSWindowBridge();
  bridged_view->OnWindowWillClose();
}

- (BOOL)windowShouldClose:(id)window {
  shell_->NotifyWindowCloseButtonClicked();
  return NO;
}

- (NSRect)window:(NSWindow*)window
    willPositionSheet:(NSWindow*)sheet
            usingRect:(NSRect)rect {
  NSView* view = window.contentView;

  rect.origin.x = shell_->GetSheetOffsetX();
  rect.origin.y = view.frame.size.height - shell_->GetSheetOffsetY();
  return rect;
}

- (void)windowWillBeginSheet:(NSNotification*)notification {
  shell_->NotifyWindowSheetBegin();
}

- (void)windowDidEndSheet:(NSNotification*)notification {
  shell_->NotifyWindowSheetEnd();
}

- (IBAction)newWindowForTab:(id)sender {
  shell_->NotifyNewWindowForTab();
  electron::Browser::Get()->NewWindowForTab();
}

#pragma mark - NSTouchBarDelegate

- (NSTouchBarItem*)touchBar:(NSTouchBar*)touchBar
      makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
    API_AVAILABLE(macosx(10.12.2)) {
  if (touchBar && shell_->touch_bar())
    return [shell_->touch_bar() makeItemForIdentifier:identifier];
  else
    return nil;
}

#pragma mark - QLPreviewPanelDataSource

- (NSInteger)numberOfPreviewItemsInPreviewPanel:(QLPreviewPanel*)panel {
  return 1;
}

- (id<QLPreviewItem>)previewPanel:(QLPreviewPanel*)panel
               previewItemAtIndex:(NSInteger)index {
  return shell_->preview_item();
}

@end
