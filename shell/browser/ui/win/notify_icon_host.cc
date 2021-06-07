// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/win/notify_icon_host.h"

#include <commctrl.h>
#include <winuser.h>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/stl_util.h"
#include "base/win/win_util.h"
#include "base/win/windows_types.h"
#include "base/win/wrapped_window_proc.h"
#include "content/public/browser/browser_task_traits.h"
#include "shell/browser/ui/win/notify_icon.h"
#include "ui/events/event_constants.h"
#include "ui/events/win/system_event_state_lookup.h"
#include "ui/gfx/win/hwnd_util.h"

namespace electron {

namespace {

const UINT kNotifyIconMessage = WM_APP + 1;

// |kBaseIconId| is 2 to avoid conflicts with plugins that hard-code id 1.
const UINT kBaseIconId = 2;

const wchar_t kNotifyIconHostWindowClass[] = L"Electron_NotifyIconHostWindow";

bool IsWinPressed() {
  return ((::GetKeyState(VK_LWIN) & 0x8000) == 0x8000) ||
         ((::GetKeyState(VK_RWIN) & 0x8000) == 0x8000);
}

int GetKeyboardModifiers() {
  int modifiers = ui::EF_NONE;
  if (ui::win::IsShiftPressed())
    modifiers |= ui::EF_SHIFT_DOWN;
  if (ui::win::IsCtrlPressed())
    modifiers |= ui::EF_CONTROL_DOWN;
  if (ui::win::IsAltPressed())
    modifiers |= ui::EF_ALT_DOWN;
  if (IsWinPressed())
    modifiers |= ui::EF_COMMAND_DOWN;
  return modifiers;
}

}  // namespace

NotifyIconHost::NotifyIconHost() {
  // Register our window class
  WNDCLASSEX window_class;
  base::win::InitializeWindowClass(
      kNotifyIconHostWindowClass,
      &base::win::WrappedWindowProc<NotifyIconHost::WndProcStatic>, 0, 0, 0,
      NULL, NULL, NULL, NULL, NULL, &window_class);
  instance_ = window_class.hInstance;
  atom_ = RegisterClassEx(&window_class);
  CHECK(atom_);

  // If the taskbar is re-created after we start up, we have to rebuild all of
  // our icons.
  taskbar_created_message_ = RegisterWindowMessage(TEXT("TaskbarCreated"));

  // Create an offscreen window for handling messages for the status icons. We
  // create a hidden WS_POPUP window instead of an HWND_MESSAGE window, because
  // only top-level windows such as popups can receive broadcast messages like
  // "TaskbarCreated".
  window_ = CreateWindow(MAKEINTATOM(atom_), 0, WS_POPUP, 0, 0, 0, 0, 0, 0,
                         instance_, 0);
  gfx::CheckWindowCreated(window_, ::GetLastError());
  gfx::SetWindowUserData(window_, this);
}

NotifyIconHost::~NotifyIconHost() {
  if (window_)
    DestroyWindow(window_);

  if (atom_)
    UnregisterClass(MAKEINTATOM(atom_), instance_);

  for (NotifyIcon* ptr : notify_icons_)
    delete ptr;
}

NotifyIcon* NotifyIconHost::CreateNotifyIcon(absl::optional<UUID> guid) {
  if (guid.has_value()) {
    for (NotifyIcons::const_iterator i(notify_icons_.begin());
         i != notify_icons_.end(); ++i) {
      auto* current_win_icon = static_cast<NotifyIcon*>(*i);
      if (current_win_icon->guid() == guid.value()) {
        LOG(WARNING)
            << "Guid already in use. Existing tray entry will be replaced.";
      }
    }
  }

  auto* notify_icon =
      new NotifyIcon(this, NextIconId(), window_, kNotifyIconMessage,
                     guid.has_value() ? guid.value() : GUID_DEFAULT);

  notify_icons_.push_back(notify_icon);
  return notify_icon;
}

void NotifyIconHost::Remove(NotifyIcon* icon) {
  NotifyIcons::iterator i(
      std::find(notify_icons_.begin(), notify_icons_.end(), icon));

  if (i == notify_icons_.end()) {
    NOTREACHED();
    return;
  }

  notify_icons_.erase(i);
}

LRESULT CALLBACK NotifyIconHost::WndProcStatic(HWND hwnd,
                                               UINT message,
                                               WPARAM wparam,
                                               LPARAM lparam) {
  auto* msg_wnd =
      reinterpret_cast<NotifyIconHost*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg_wnd)
    return msg_wnd->WndProc(hwnd, message, wparam, lparam);
  else
    return ::DefWindowProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK NotifyIconHost::WndProc(HWND hwnd,
                                         UINT message,
                                         WPARAM wparam,
                                         LPARAM lparam) {
  if (message == taskbar_created_message_) {
    // We need to reset all of our icons because the taskbar went away.
    for (NotifyIcons::const_iterator i(notify_icons_.begin());
         i != notify_icons_.end(); ++i) {
      auto* win_icon = static_cast<NotifyIcon*>(*i);
      win_icon->ResetIcon();
    }
    return TRUE;
  } else if (message == kNotifyIconMessage) {
    NotifyIcon* win_icon = NULL;

    // Find the selected status icon.
    for (NotifyIcons::const_iterator i(notify_icons_.begin());
         i != notify_icons_.end(); ++i) {
      auto* current_win_icon = static_cast<NotifyIcon*>(*i);
      if (current_win_icon->icon_id() == wparam) {
        win_icon = current_win_icon;
        break;
      }
    }

    // It is possible for this procedure to be called with an obsolete icon
    // id.  In that case we should just return early before handling any
    // actions.
    if (!win_icon)
      return TRUE;

    // We use a WeakPtr factory for NotifyIcons here so
    // that the callback is aware if the NotifyIcon gets
    // garbage-collected. This occurs when the tray gets
    // GC'd, and the BALLOON events below will not emit.
    base::WeakPtr<NotifyIcon> win_icon_weak = win_icon->GetWeakPtr();

    switch (lparam) {
      case NIN_BALLOONSHOW:
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce(&NotifyIcon::NotifyBalloonShow, win_icon_weak));
        return TRUE;

      case NIN_BALLOONUSERCLICK:
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce(&NotifyIcon::NotifyBalloonClicked, win_icon_weak));
        return TRUE;

      case NIN_BALLOONTIMEOUT:
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce(&NotifyIcon::NotifyBalloonClosed, win_icon_weak));
        return TRUE;

      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDBLCLK:
      case WM_CONTEXTMENU:
        // Walk our icons, find which one was clicked on, and invoke its
        // HandleClickEvent() method.
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce(
                &NotifyIcon::HandleClickEvent, win_icon_weak,
                GetKeyboardModifiers(),
                (lparam == WM_LBUTTONDOWN || lparam == WM_LBUTTONDBLCLK),
                (lparam == WM_LBUTTONDBLCLK || lparam == WM_RBUTTONDBLCLK)));

        return TRUE;

      case WM_MOUSEMOVE:
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE, base::BindOnce(&NotifyIcon::HandleMouseMoveEvent,
                                      win_icon_weak, GetKeyboardModifiers()));
        return TRUE;
    }
  }
  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

UINT NotifyIconHost::NextIconId() {
  UINT icon_id = next_icon_id_++;
  return kBaseIconId + icon_id;
}

}  // namespace electron
