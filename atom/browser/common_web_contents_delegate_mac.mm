// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/common_web_contents_delegate.h"

#import <Cocoa/Cocoa.h>

#include "content/public/browser/native_web_keyboard_event.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace atom {

void CommonWebContentsDelegate::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (event.skip_in_browser ||
      event.type == content::NativeWebKeyboardEvent::Char)
    return;

  // Escape exits tabbed fullscreen mode.
  if (event.windowsKeyCode == ui::VKEY_ESCAPE && is_html_fullscreen())
    ExitFullscreenModeForTab(source);

  BOOL handled = [[NSApp mainMenu] performKeyEquivalent:event.os_event];
  if (!handled && event.os_event.window) {
    // Handle the cmd+~ shortcut.
    if ((event.os_event.modifierFlags & NSCommandKeyMask) /* cmd */ &&
        (event.os_event.keyCode == 50  /* ~ */)) {
      if (event.os_event.modifierFlags & NSShiftKeyMask) {
        [NSApp sendAction:@selector(_cycleWindowsReversed:) to:nil from:nil];
      } else {
        [NSApp sendAction:@selector(_cycleWindows:) to:nil from:nil];
      }
    }
  }
}

}  // namespace atom
