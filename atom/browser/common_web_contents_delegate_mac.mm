// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/common_web_contents_delegate.h"

#import <Cocoa/Cocoa.h>

#include "atom/browser/web_contents_preferences.h"
#include "brightray/browser/mac/event_dispatching_window.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "ui/events/keycodes/keyboard_codes.h"

@interface NSWindow (EventDispatchingWindow)
- (void)redispatchKeyEvent:(NSEvent*)event;
@end

namespace atom {

void CommonWebContentsDelegate::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (event.skip_in_browser ||
      event.GetType() == content::NativeWebKeyboardEvent::kChar)
    return;

  // Escape exits tabbed fullscreen mode.
  if (event.windows_key_code == ui::VKEY_ESCAPE && is_html_fullscreen())
    ExitFullscreenModeForTab(source);

  // Check if the webContents has preferences and to ignore shortcuts
  auto* web_preferences = WebContentsPreferences::From(source);
  if (web_preferences &&
      web_preferences->IsEnabled("ignoreMenuShortcuts", false))
    return;

  // Send the event to the menu before sending it to the window
  if (event.os_event.type == NSKeyDown &&
      [[NSApp mainMenu] performKeyEquivalent:event.os_event])
    return;

  if (event.os_event.window &&
      [event.os_event.window isKindOfClass:[EventDispatchingWindow class]])
    [event.os_event.window redispatchKeyEvent:event.os_event];
}

}  // namespace atom
