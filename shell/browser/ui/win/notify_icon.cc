// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/win/notify_icon.h"

#include <objbase.h>

#include "base/containers/span.h"
#include "base/logging.h"
#include "base/strings/string_util_win.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/ui/win/notify_icon_host.h"
#include "ui/base/mojom/menu_source_type.mojom.h"
#include "ui/display/screen.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace {

UINT ConvertIconType(electron::TrayIcon::IconType type) {
  using IconType = electron::TrayIcon::IconType;
  switch (type) {
    case IconType::kNone:
      return NIIF_NONE;
    case IconType::kInfo:
      return NIIF_INFO;
    case IconType::kWarning:
      return NIIF_WARNING;
    case IconType::kError:
      return NIIF_ERROR;
    case IconType::kCustom:
      return NIIF_USER;
    default:
      NOTREACHED() << "Invalid icon type";
  }
}

}  // namespace

namespace electron {

NotifyIcon::NotifyIcon(NotifyIconHost* host,
                       UINT id,
                       HWND window,
                       UINT message,
                       GUID guid)
    : host_(host), icon_id_(id), window_(window), message_id_(message) {
  guid_ = guid;
  is_using_guid_ = guid != GUID_DEFAULT;
  NOTIFYICONDATA icon_data = InitIconData();
  icon_data.uFlags |= NIF_MESSAGE;
  icon_data.uCallbackMessage = message_id_;
  const BOOL result = Shell_NotifyIcon(NIM_ADD, &icon_data);
  // This can happen if the explorer process isn't running when we try to
  // create the icon for some reason (for example, at startup).
  if (!result)
    LOG(WARNING) << "Unable to create status tray icon.";
}

NotifyIcon::~NotifyIcon() {
  // Remove our icon.
  host_->Remove(this);
  NOTIFYICONDATA icon_data = InitIconData();
  Shell_NotifyIcon(NIM_DELETE, &icon_data);
}

void NotifyIcon::HandleClickEvent(int modifiers,
                                  bool left_mouse_click,
                                  bool double_button_click,
                                  bool middle_button_click) {
  gfx::Rect bounds = GetBounds();

  if (left_mouse_click) {
    if (double_button_click)  // double left click
      NotifyDoubleClicked(bounds, modifiers);
    else  // single left click
      NotifyClicked(bounds, display::Screen::Get()->GetCursorScreenPoint(),
                    modifiers);
    return;
  } else if (middle_button_click) {  // single middle click
    NotifyMiddleClicked(bounds, modifiers);
  } else if (!double_button_click) {  // single right click
    if (menu_model_)
      PopUpContextMenu(gfx::Point(), menu_model_->GetWeakPtr());
    else
      NotifyRightClicked(bounds, modifiers);
  }
}

void NotifyIcon::HandleMouseMoveEvent(int modifiers) {
  gfx::Point cursorPos = display::Screen::Get()->GetCursorScreenPoint();
  // Omit event fired when tray icon is created but cursor is outside of it.
  if (GetBounds().Contains(cursorPos))
    NotifyMouseMoved(cursorPos, modifiers);
}

void NotifyIcon::HandleMouseEntered(int modifiers) {
  gfx::Point cursor_pos = display::Screen::Get()->GetCursorScreenPoint();
  NotifyMouseEntered(cursor_pos, modifiers);
}

void NotifyIcon::HandleMouseExited(int modifiers) {
  gfx::Point cursor_pos = display::Screen::Get()->GetCursorScreenPoint();
  NotifyMouseExited(cursor_pos, modifiers);
}

void NotifyIcon::ResetIcon() {
  // Delete any previously existing icon.
  NOTIFYICONDATA icon_data = InitIconData();
  Shell_NotifyIcon(NIM_DELETE, &icon_data);

  // Update the icon.
  icon_data = InitIconData();
  icon_data.uFlags |= NIF_MESSAGE;
  icon_data.uCallbackMessage = message_id_;
  icon_data.hIcon = icon_.get();
  // If we have an image, then set the NIF_ICON flag, which tells
  // Shell_NotifyIcon() to set the image for the status icon it creates.
  if (icon_data.hIcon)
    icon_data.uFlags |= NIF_ICON;
  // Re-add our icon.
  BOOL result = Shell_NotifyIcon(NIM_ADD, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to re-create status tray icon.";
}

void NotifyIcon::SetImage(HICON image) {
  icon_ = base::win::ScopedGDIObject<HICON>(CopyIcon(image));

  // Create the icon.
  NOTIFYICONDATA icon_data = InitIconData();
  icon_data.uFlags |= NIF_ICON;
  icon_data.hIcon = image;
  BOOL result = Shell_NotifyIcon(NIM_MODIFY, &icon_data);
  if (!result)
    LOG(WARNING) << "Error setting status tray icon image";
}

void NotifyIcon::SetPressedImage(HICON image) {
  // Ignore pressed images, since the standard on Windows is to not highlight
  // pressed status icons.
}

template <typename CharT, size_t N>
static void CopyStringToBuf(CharT (&tgt_buf)[N],
                            const std::basic_string<CharT> src_str) {
  if constexpr (N < 1U)
    return;

  const auto src = base::span{src_str};
  const auto n_chars = std::min(src.size(), N - 1U);
  auto tgt = base::span{tgt_buf};
  tgt.first(n_chars).copy_from(src.first(n_chars));
  tgt[n_chars] = CharT{};  // zero-terminate the string
}

void NotifyIcon::SetToolTip(const std::string& tool_tip) {
  // Create the icon.
  NOTIFYICONDATA icon_data = InitIconData();
  icon_data.uFlags |= NIF_TIP;
  CopyStringToBuf(icon_data.szTip, base::UTF8ToWide(tool_tip));
  BOOL result = Shell_NotifyIcon(NIM_MODIFY, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to set tooltip for status tray icon";
}

void NotifyIcon::DisplayBalloon(const BalloonOptions& options) {
  NOTIFYICONDATA icon_data = InitIconData();
  icon_data.uFlags |= NIF_INFO;
  CopyStringToBuf(icon_data.szInfoTitle, base::AsWString(options.title));
  CopyStringToBuf(icon_data.szInfo, base::AsWString(options.content));
  icon_data.uTimeout = 0;
  icon_data.hBalloonIcon = options.icon;
  icon_data.dwInfoFlags = ConvertIconType(options.icon_type);

  if (options.large_icon)
    icon_data.dwInfoFlags |= NIIF_LARGE_ICON;

  if (options.no_sound)
    icon_data.dwInfoFlags |= NIIF_NOSOUND;

  if (options.respect_quiet_time)
    icon_data.dwInfoFlags |= NIIF_RESPECT_QUIET_TIME;

  BOOL result = Shell_NotifyIcon(NIM_MODIFY, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to create status tray balloon.";
}

void NotifyIcon::RemoveBalloon() {
  NOTIFYICONDATA icon_data = InitIconData();
  icon_data.uFlags |= NIF_INFO;

  BOOL result = Shell_NotifyIcon(NIM_MODIFY, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to remove status tray balloon.";
}

void NotifyIcon::Focus() {
  NOTIFYICONDATA icon_data = InitIconData();

  BOOL result = Shell_NotifyIcon(NIM_SETFOCUS, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to focus tray icon.";
}

void NotifyIcon::PopUpContextMenu(const gfx::Point& pos,
                                  base::WeakPtr<ElectronMenuModel> menu_model) {
  // Returns if context menu isn't set.
  if (menu_model == nullptr && menu_model_ == nullptr)
    return;

  // Set our window as the foreground window, so the context menu closes when
  // we click away from it.
  if (!SetForegroundWindow(window_))
    return;

  // Cancel current menu if there is one.
  CloseContextMenu();

  // Show menu at mouse's position by default.
  gfx::Rect rect(pos, gfx::Size());
  if (pos.IsOrigin())
    rect.set_origin(display::Screen::Get()->GetCursorScreenPoint());

  if (menu_model) {
    menu_runner_ = std::make_unique<views::MenuRunner>(
        menu_model.get(), views::MenuRunner::HAS_MNEMONICS);
  } else {
    menu_runner_ = std::make_unique<views::MenuRunner>(
        menu_model_, views::MenuRunner::HAS_MNEMONICS);
  }
  menu_runner_->RunMenuAt(nullptr, nullptr, rect,
                          views::MenuAnchorPosition::kTopLeft,
                          ui::mojom::MenuSourceType::kMouse);
}

void NotifyIcon::CloseContextMenu() {
  if (menu_runner_ && menu_runner_->IsRunning()) {
    menu_runner_->Cancel();
  }
}

void NotifyIcon::SetContextMenu(raw_ptr<ElectronMenuModel> menu_model) {
  menu_model_ = menu_model;
}

gfx::Rect NotifyIcon::GetBounds() {
  NOTIFYICONIDENTIFIER icon_id = {};
  icon_id.uID = icon_id_;
  icon_id.hWnd = window_;
  icon_id.cbSize = sizeof(NOTIFYICONIDENTIFIER);
  if (is_using_guid_) {
    icon_id.guidItem = guid_;
  }

  RECT rect = {0};
  Shell_NotifyIconGetRect(&icon_id, &rect);
  return display::win::GetScreenWin()->ScreenToDIPRect(window_,
                                                       gfx::Rect{rect});
}

NOTIFYICONDATA NotifyIcon::InitIconData() const {
  NOTIFYICONDATA icon_data = {};
  icon_data.cbSize = sizeof(NOTIFYICONDATA);
  icon_data.hWnd = window_;
  icon_data.uID = icon_id_;
  if (is_using_guid_) {
    icon_data.uFlags = NIF_GUID;
    icon_data.guidItem = guid_;
  }
  return icon_data;
}

}  // namespace electron
