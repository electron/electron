// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "shell/app_shim/app_shim_window.h"

#include <objc/runtime.h>

#include "base/strings/sys_string_conversions.h"
#import "components/remote_cocoa/app_shim/native_widget_mac_nswindow.h"
#include "components/remote_cocoa/app_shim/native_widget_ns_window_bridge.h"
#include "shell/browser/native_window.h"
#import "shell/browser/ui/cocoa/electron_preview_item.h"
#import "shell/browser/ui/cocoa/window_buttons_proxy.h"

namespace {

using TitleBarStyle = electron::NativeWindow::TitleBarStyle;

const char kAssociationKey = 0;

// 0 for a name that is not a vibrancy type.
NSVisualEffectMaterial MaterialForVibrancyType(const std::string& type) {
  static const struct {
    const char* type;
    NSVisualEffectMaterial material;
  } kMaterials[] = {
      {"titlebar", NSVisualEffectMaterialTitlebar},
      {"selection", NSVisualEffectMaterialSelection},
      {"menu", NSVisualEffectMaterialMenu},
      {"popover", NSVisualEffectMaterialPopover},
      {"sidebar", NSVisualEffectMaterialSidebar},
      {"header", NSVisualEffectMaterialHeaderView},
      {"sheet", NSVisualEffectMaterialSheet},
      {"window", NSVisualEffectMaterialWindowBackground},
      {"hud", NSVisualEffectMaterialHUDWindow},
      {"fullscreen-ui", NSVisualEffectMaterialFullScreenUI},
      {"tooltip", NSVisualEffectMaterialToolTip},
      {"content", NSVisualEffectMaterialContentBackground},
      {"under-window", NSVisualEffectMaterialUnderWindowBackground},
      {"under-page", NSVisualEffectMaterialUnderPageBackground},
  };
  for (const auto& entry : kMaterials) {
    if (type == entry.type)
      return entry.material;
  }
  return static_cast<NSVisualEffectMaterial>(0);
}

}  // namespace

@implementation ElectronAppShimWindow {
  NSWindow* __weak _window;
  BOOL _hasFrame;
  TitleBarStyle _titleBarStyle;
  WindowButtonsProxy* __strong _buttonsProxy;
  BOOL _buttonsVisible;
  NSVisualEffectView* __strong _vibrantView;
  ElectronPreviewItem* __strong _previewItem;
}

+ (instancetype)forWindowId:(uint64_t)windowId {
  auto* bridge = remote_cocoa::NativeWidgetNSWindowBridge::GetFromId(windowId);
  NSWindow* window = bridge ? bridge->ns_window() : nil;
  if (!window)
    return nil;
  ElectronAppShimWindow* existing = [self existingForWindow:window];
  if (existing)
    return existing;
  ElectronAppShimWindow* made = [[self alloc] initWithWindow:window];
  objc_setAssociatedObject(window, &kAssociationKey, made,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  return made;
}

+ (instancetype)existingForWindow:(NSWindow*)window {
  return window ? objc_getAssociatedObject(window, &kAssociationKey) : nil;
}

- (instancetype)initWithWindow:(NSWindow*)window {
  if ((self = [super init])) {
    _window = window;
    _hasFrame = YES;
    _titleBarStyle = TitleBarStyle::kNormal;
    _buttonsVisible = YES;
    NSNotificationCenter* center = NSNotificationCenter.defaultCenter;
    [center addObserver:self
               selector:@selector(windowDidResize:)
                   name:NSWindowDidResizeNotification
                 object:window];
    for (NSNotificationName name in @[
           NSWindowDidBecomeMainNotification, NSWindowDidResignMainNotification,
           NSWindowDidBecomeKeyNotification, NSWindowDidResignKeyNotification,
           NSWindowDidChangeOcclusionStateNotification
         ]) {
      [center addObserver:self
                 selector:@selector(windowDidResize:)
                     name:name
                   object:window];
    }
    [center addObserver:self
               selector:@selector(windowDidEnterFullScreen:)
                   name:NSWindowDidEnterFullScreenNotification
                 object:window];
    [center addObserver:self
               selector:@selector(windowWillExitFullScreen:)
                   name:NSWindowWillExitFullScreenNotification
                 object:window];
    [center addObserver:self
               selector:@selector(windowDidExitFullScreen:)
                   name:NSWindowDidExitFullScreenNotification
                 object:window];
  }
  return self;
}

- (void)dealloc {
  [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)setStandardButtonsHidden:(BOOL)hidden {
  [_window standardWindowButton:NSWindowCloseButton].hidden = hidden;
  [_window standardWindowButton:NSWindowMiniaturizeButton].hidden = hidden;
  [_window standardWindowButton:NSWindowZoomButton].hidden = hidden;
}

- (void)initChrome:(const electron::mojom::AppShimWindowChrome&)chrome {
  _hasFrame = chrome.has_frame;
  _titleBarStyle = static_cast<TitleBarStyle>(chrome.title_bar_style);

  if (chrome.tabbing_identifier && _hasFrame) {
    _window.tabbingIdentifier =
        base::SysUTF8ToNSString(*chrome.tabbing_identifier);
  } else {
    _window.tabbingMode = NSWindowTabbingModeDisallowed;
  }

  if (_hasFrame)
    return;
  // The window has a title bar that is see-through and has no title. Whether
  // its buttons show is the title bar style's to say.
  if (_titleBarStyle == TitleBarStyle::kNormal) {
    [self setStandardButtonsHidden:YES];
    return;
  }
  _buttonsProxy = [[WindowButtonsProxy alloc] initWithWindow:_window];
  [_buttonsProxy setHeight:chrome.titlebar_overlay_height];
  if (chrome.traffic_light_position) {
    [_buttonsProxy setMargin:*chrome.traffic_light_position];
  } else if (_titleBarStyle == TitleBarStyle::kHiddenInset) {
    [_buttonsProxy setMargin:gfx::Point(12, 11)];
  }
  if (_titleBarStyle == TitleBarStyle::kCustomButtonsOnHover) {
    [_buttonsProxy setShowOnHover:YES];
  } else {
    [self setStandardButtonsHidden:NO];
  }
}

- (void)setButtonVisibility:(BOOL)visible {
  _buttonsVisible = visible;
  if (_buttonsProxy) {
    if (visible)
      [_buttonsProxy redraw];
    [_buttonsProxy setVisible:visible];
  }
  if (_titleBarStyle != TitleBarStyle::kCustomButtonsOnHover)
    [self setStandardButtonsHidden:!visible];
}

- (void)setButtonPosition:(const std::optional<gfx::Point>&)position {
  [_buttonsProxy setMargin:position];
}

// Also when the window comes on screen, and when it becomes or stops being
// the main or key window: each lays the title bar out again, which puts the
// buttons back where the system has them. The layout may come after the
// notification, so the buttons are placed once more behind it.
- (void)windowDidResize:(NSNotification*)notification {
  [self redrawButtons];
  ElectronAppShimWindow* __weak weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    [weakSelf redrawButtons];
  });
}

- (void)redrawButtons {
  if (!(_window.styleMask & NSWindowStyleMaskFullScreen))
    [_buttonsProxy redraw];
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
  // The title comes back for full screen, where the title bar slides in.
  if (_buttonsProxy)
    _window.titleVisibility = NSWindowTitleVisible;
  if (!_hasFrame)
    _window.titlebarAppearsTransparent = NO;
}

- (void)windowWillExitFullScreen:(NSNotification*)notification {
  if (_buttonsProxy) {
    _window.titleVisibility = NSWindowTitleHidden;
    // Hidden meanwhile, or the buttons jump.
    [_buttonsProxy setVisible:NO];
  }
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
  if (_buttonsProxy && _buttonsVisible) {
    [_buttonsProxy redraw];
    [_buttonsProxy setVisible:YES];
  }
  if (!_hasFrame)
    _window.titlebarAppearsTransparent = YES;
}

- (void)setVibrancy:(const std::string&)type
           duration:(int)durationMs
              state:(NSVisualEffectState)state {
  const BOOL animate = durationMs > 0;
  NSVisualEffectView* vibrantView = _vibrantView;

  if (type.empty()) {
    if (!vibrantView)
      return;
    _vibrantView = nil;
    if (animate) {
      [NSAnimationContext
          runAnimationGroup:^(NSAnimationContext* context) {
            context.duration = durationMs / 1000.0;
            vibrantView.animator.alphaValue = 0.0;
          }
          completionHandler:^{
            [vibrantView removeFromSuperview];
          }];
    } else {
      [vibrantView removeFromSuperview];
    }
    return;
  }

  const NSVisualEffectMaterial material = MaterialForVibrancyType(type);
  if (!material)
    return;

  if (!vibrantView) {
    NSView* contentView = _window.contentView;
    vibrantView = [[NSVisualEffectView alloc] initWithFrame:contentView.bounds];
    vibrantView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    vibrantView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    vibrantView.state = state;
    // Underneath everything else in the window.
    [contentView addSubview:vibrantView
                 positioned:NSWindowBelow
                 relativeTo:nil];
    _vibrantView = vibrantView;
    if (animate) {
      vibrantView.alphaValue = 0.0;
      [NSAnimationContext
          runAnimationGroup:^(NSAnimationContext* context) {
            context.duration = durationMs / 1000.0;
            vibrantView.animator.alphaValue = 1.0;
          }
          completionHandler:nil];
    }
  }
  vibrantView.material = material;
}

- (void)performTabAction:(electron::mojom::AppShimTabAction)action {
  using Action = electron::mojom::AppShimTabAction;
  switch (action) {
    case Action::kSelectPrevious:
      [_window selectPreviousTab:nil];
      break;
    case Action::kSelectNext:
      [_window selectNextTab:nil];
      break;
    case Action::kShowAll:
      [_window toggleTabOverview:nil];
      break;
    case Action::kMergeAll:
      [_window mergeAllWindows:nil];
      break;
    case Action::kMoveToNewWindow:
      [_window moveTabToNewWindow:nil];
      break;
    case Action::kToggleTabBar:
      [_window toggleTabBar:nil];
      break;
  }
}

- (void)addTabbedWindow:(ElectronAppShimWindow*)other {
  NSWindow* otherWindow = other ? other->_window : nil;
  if (otherWindow && otherWindow != _window)
    [_window addTabbedWindow:otherWindow ordered:NSWindowAbove];
}

- (void)previewFile:(NSURL*)url title:(NSString*)title {
  _previewItem = [[ElectronPreviewItem alloc] initWithURL:url title:title];
  [_window makeKeyWindow];
  QLPreviewPanel* panel = [QLPreviewPanel sharedPreviewPanel];
  // The panel asks the responder chain who fills it only when it opens.
  if (panel.visible)
    [panel reloadData];
  [panel makeKeyAndOrderFront:nil];
}

- (void)closeFilePreview {
  // Asking for the shared panel makes one, which would show it.
  if ([QLPreviewPanel sharedPreviewPanelExists] && _previewItem)
    [[QLPreviewPanel sharedPreviewPanel] close];
  _previewItem = nil;
}

- (BOOL)hasPreviewItem {
  return _previewItem != nil;
}

// QLPreviewPanelDataSource

- (NSInteger)numberOfPreviewItemsInPreviewPanel:(QLPreviewPanel*)panel {
  return _previewItem ? 1 : 0;
}

- (id<QLPreviewItem>)previewPanel:(QLPreviewPanel*)panel
               previewItemAtIndex:(NSInteger)index {
  return _previewItem;
}

@end
