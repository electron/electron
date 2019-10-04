// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/common_web_contents_delegate.h"

#include "base/strings/string_util.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "shell/browser/api/atom_api_web_contents.h"
#include "shell/browser/native_window_views.h"
#include "ui/events/keycodes/keyboard_codes.h"

#if defined(USE_X11)
#include "shell/browser/browser.h"
#endif

namespace electron {

content::KeyboardEventProcessingResult
CommonWebContentsDelegate::PreHandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (menu_shortcut_priority_ == MenuShortcutPriority::FIRST &&
      owner_window() && owner_window()->HandleKeyboardEventForMenu(event)) {
    return content::KeyboardEventProcessingResult::HANDLED;
  }

  return content::KeyboardEventProcessingResult::NOT_HANDLED;
}

bool CommonWebContentsDelegate::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  // Escape exits tabbed fullscreen mode.
  if (event.windows_key_code == ui::VKEY_ESCAPE && is_html_fullscreen()) {
    ExitFullscreenModeForTab(source);
    return true;
  }

  // Let the NativeWindow handle other parts.
  if (menu_shortcut_priority_ == MenuShortcutPriority::LAST && owner_window()) {
    owner_window()->HandleKeyboardEvent(source, event);
    return true;
  }

  return false;
}

gfx::ImageSkia CommonWebContentsDelegate::GetDevToolsWindowIcon() {
  if (!owner_window())
    return gfx::ImageSkia();
  return static_cast<views::WidgetDelegate*>(
             static_cast<NativeWindowViews*>(owner_window()))
      ->GetWindowAppIcon();
}

#if defined(USE_X11)
void CommonWebContentsDelegate::GetDevToolsWindowWMClass(
    std::string* name,
    std::string* class_name) {
  *class_name = Browser::Get()->GetName();
  *name = base::ToLowerASCII(*class_name);
}
#endif

}  // namespace electron
