// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/common_web_contents_delegate.h"

#include "base/strings/string_util.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "shell/browser/api/electron_api_web_contents_view.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/web_contents_preferences.h"
#include "ui/events/keycodes/keyboard_codes.h"

#if defined(USE_X11)
#include "shell/browser/browser.h"
#endif

namespace electron {

bool CommonWebContentsDelegate::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  // Escape exits tabbed fullscreen mode.
  if (event.windows_key_code == ui::VKEY_ESCAPE && is_html_fullscreen()) {
    ExitFullscreenModeForTab(source);
    return true;
  }

  // Check if the webContents has preferences and to ignore shortcuts
  auto* web_preferences = WebContentsPreferences::From(source);
  if (web_preferences &&
      web_preferences->IsEnabled("ignoreMenuShortcuts", false))
    return false;

  // Let the NativeWindow handle other parts.
  if (owner_window()) {
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
