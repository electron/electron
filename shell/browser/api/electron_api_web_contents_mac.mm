// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "content/public/browser/render_widget_host_view.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/ui/cocoa/event_dispatching_window.h"
#include "shell/browser/web_contents_preferences.h"
#include "ui/base/cocoa/command_dispatcher.h"
#include "ui/events/keycodes/keyboard_codes.h"

#import <Cocoa/Cocoa.h>

@interface NSWindow (EventDispatchingWindow)
- (void)redispatchKeyEvent:(NSEvent*)event;
@end

namespace electron::api {

bool WebContents::IsFocused() const {
  auto* view = web_contents()->GetRenderWidgetHostView();
  if (!view)
    return false;

  if (GetType() != Type::kBackgroundPage) {
    auto window = [web_contents()->GetNativeView().GetNativeNSView() window];
    // On Mac the render widget host view does not lose focus when the window
    // loses focus so check if the top level window is the key window.
    if (window && ![window isKeyWindow])
      return false;
  }

  return view->HasFocus();
}

bool WebContents::PlatformHandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (event.skip_in_browser ||
      event.GetType() == content::NativeWebKeyboardEvent::Type::kChar)
    return false;

  // Escape exits tabbed fullscreen mode.
  if (event.windows_key_code == ui::VKEY_ESCAPE && is_html_fullscreen()) {
    ExitFullscreenModeForTab(source);
    return true;
  }

  // Check if the webContents has preferences and to ignore shortcuts
  auto* web_preferences = WebContentsPreferences::From(source);
  if (web_preferences && web_preferences->ShouldIgnoreMenuShortcuts())
    return false;

  // Send the event to the menu before sending it to the window
  if (event.os_event.type == NSEventTypeKeyDown &&
      [[NSApp mainMenu] performKeyEquivalent:event.os_event])
    return true;

  // Let the window redispatch the OS event
  if (event.os_event.window &&
      [event.os_event.window isKindOfClass:[EventDispatchingWindow class]]) {
    [event.os_event.window redispatchKeyEvent:event.os_event];
    // FIXME(nornagon): this isn't the right return value; we should implement
    // devtools windows as Widgets in order to take advantage of the
    // preexisting redispatch code in bridged_native_widget.
    return false;
  } else if (event.os_event.window &&
             [event.os_event.window
                 conformsToProtocol:@protocol(CommandDispatchingWindow)]) {
    NSObject<CommandDispatchingWindow>* window =
        static_cast<NSObject<CommandDispatchingWindow>*>(event.os_event.window);
    [[window commandDispatcher] redispatchKeyEvent:event.os_event];
    // FIXME(clavin): Not exactly sure what to return here, likely the same
    // situation as the branch above. If a future refactor removes
    // |EventDispatchingWindow| then only this branch will need to remain.
    return false;
  }

  return false;
}

}  // namespace electron::api
