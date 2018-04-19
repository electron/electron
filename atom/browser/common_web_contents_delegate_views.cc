// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/common_web_contents_delegate.h"

#include "atom/browser/native_window_views.h"
#include "base/strings/string_util.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "ui/events/keycodes/keyboard_codes.h"

#if defined(USE_X11)
#include "atom/browser/browser.h"
#endif

namespace atom {

void CommonWebContentsDelegate::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  // Escape exits tabbed fullscreen mode.
  if (event.windows_key_code == ui::VKEY_ESCAPE && is_html_fullscreen())
    ExitFullscreenModeForTab(source);

  // Let the NativeWindow handle other parts.
  if (!ignore_menu_shortcuts_ && owner_window())
    owner_window()->HandleKeyboardEvent(source, event);
}

void CommonWebContentsDelegate::ShowAutofillPopup(
    bool offscreen,
    content::RenderFrameHost* frame_host,
    const gfx::RectF& bounds,
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  if (!owner_window())
    return;

  auto* window = static_cast<NativeWindowViews*>(owner_window());
  autofill_popup_->CreateView(frame_host, offscreen, window->content_view(),
                              bounds);
  autofill_popup_->SetItems(values, labels);
}

void CommonWebContentsDelegate::HideAutofillPopup() {
  if (autofill_popup_)
    autofill_popup_->Hide();
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

}  // namespace atom
