// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <dwmapi.h>
#include <shellapi.h>
#include <wrl/client.h>

#include "base/win/atl.h"  // Must be before UIAutomationCore.h
#include "base/win/registry.h"
#include "base/win/scoped_handle.h"
#include "base/win/windows_version.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/views/root_view.h"
#include "shell/browser/ui/views/win_frame_view.h"
#include "shell/common/color_util.h"
#include "shell/common/electron_constants.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/resize_utils.h"

// Must be included after other Windows headers.
#include <UIAutomationClient.h>
#include <UIAutomationCoreApi.h>

namespace electron {

namespace {

void SetWindowBorderAndCaptionColor(HWND hwnd, COLORREF color, bool has_frame) {
  HRESULT result;
  if (has_frame) {
    result =
        DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &color, sizeof(color));

    if (FAILED(result))
      LOG(WARNING) << "Failed to set caption color";
  }

  result =
      DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &color, sizeof(color));

  if (FAILED(result))
    LOG(WARNING) << "Failed to set border color";
}

bool IsAccentColorOnTitleBarsEnabled() {
  base::win::RegKey key;
  if (key.Open(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\DWM",
               KEY_READ) != ERROR_SUCCESS) {
    return false;
  }

  DWORD enabled = 0;
  if (key.ReadValueDW(L"ColorPrevalence", &enabled) != ERROR_SUCCESS) {
    return false;
  }

  return enabled != 0;
}

// Convert Win32 WM_QUERYENDSESSIONS to strings.
const std::vector<std::string> EndSessionToStringVec(LPARAM end_session_id) {
  std::vector<std::string> params;
  if (end_session_id == 0) {
    params.push_back("shutdown");
    return params;
  }

  if (end_session_id & ENDSESSION_CLOSEAPP)
    params.push_back("close-app");
  if (end_session_id & ENDSESSION_CRITICAL)
    params.push_back("critical");
  if (end_session_id & ENDSESSION_LOGOFF)
    params.push_back("logoff");

  return params;
}

// Convert Win32 WM_APPCOMMANDS to strings.
constexpr std::string_view AppCommandToString(int command_id) {
  switch (command_id) {
    case APPCOMMAND_BROWSER_BACKWARD:
      return kBrowserBackward;
    case APPCOMMAND_BROWSER_FORWARD:
      return kBrowserForward;
    case APPCOMMAND_BROWSER_REFRESH:
      return "browser-refresh";
    case APPCOMMAND_BROWSER_STOP:
      return "browser-stop";
    case APPCOMMAND_BROWSER_SEARCH:
      return "browser-search";
    case APPCOMMAND_BROWSER_FAVORITES:
      return "browser-favorites";
    case APPCOMMAND_BROWSER_HOME:
      return "browser-home";
    case APPCOMMAND_VOLUME_MUTE:
      return "volume-mute";
    case APPCOMMAND_VOLUME_DOWN:
      return "volume-down";
    case APPCOMMAND_VOLUME_UP:
      return "volume-up";
    case APPCOMMAND_MEDIA_NEXTTRACK:
      return "media-nexttrack";
    case APPCOMMAND_MEDIA_PREVIOUSTRACK:
      return "media-previoustrack";
    case APPCOMMAND_MEDIA_STOP:
      return "media-stop";
    case APPCOMMAND_MEDIA_PLAY_PAUSE:
      return "media-play-pause";
    case APPCOMMAND_LAUNCH_MAIL:
      return "launch-mail";
    case APPCOMMAND_LAUNCH_MEDIA_SELECT:
      return "launch-media-select";
    case APPCOMMAND_LAUNCH_APP1:
      return "launch-app1";
    case APPCOMMAND_LAUNCH_APP2:
      return "launch-app2";
    case APPCOMMAND_BASS_DOWN:
      return "bass-down";
    case APPCOMMAND_BASS_BOOST:
      return "bass-boost";
    case APPCOMMAND_BASS_UP:
      return "bass-up";
    case APPCOMMAND_TREBLE_DOWN:
      return "treble-down";
    case APPCOMMAND_TREBLE_UP:
      return "treble-up";
    case APPCOMMAND_MICROPHONE_VOLUME_MUTE:
      return "microphone-volume-mute";
    case APPCOMMAND_MICROPHONE_VOLUME_DOWN:
      return "microphone-volume-down";
    case APPCOMMAND_MICROPHONE_VOLUME_UP:
      return "microphone-volume-up";
    case APPCOMMAND_HELP:
      return "help";
    case APPCOMMAND_FIND:
      return "find";
    case APPCOMMAND_NEW:
      return "new";
    case APPCOMMAND_OPEN:
      return "open";
    case APPCOMMAND_CLOSE:
      return "close";
    case APPCOMMAND_SAVE:
      return "save";
    case APPCOMMAND_PRINT:
      return "print";
    case APPCOMMAND_UNDO:
      return "undo";
    case APPCOMMAND_REDO:
      return "redo";
    case APPCOMMAND_COPY:
      return "copy";
    case APPCOMMAND_CUT:
      return "cut";
    case APPCOMMAND_PASTE:
      return "paste";
    case APPCOMMAND_REPLY_TO_MAIL:
      return "reply-to-mail";
    case APPCOMMAND_FORWARD_MAIL:
      return "forward-mail";
    case APPCOMMAND_SEND_MAIL:
      return "send-mail";
    case APPCOMMAND_SPELL_CHECK:
      return "spell-check";
    case APPCOMMAND_MIC_ON_OFF_TOGGLE:
      return "mic-on-off-toggle";
    case APPCOMMAND_CORRECTION_LIST:
      return "correction-list";
    case APPCOMMAND_MEDIA_PLAY:
      return "media-play";
    case APPCOMMAND_MEDIA_PAUSE:
      return "media-pause";
    case APPCOMMAND_MEDIA_RECORD:
      return "media-record";
    case APPCOMMAND_MEDIA_FAST_FORWARD:
      return "media-fast-forward";
    case APPCOMMAND_MEDIA_REWIND:
      return "media-rewind";
    case APPCOMMAND_MEDIA_CHANNEL_UP:
      return "media-channel-up";
    case APPCOMMAND_MEDIA_CHANNEL_DOWN:
      return "media-channel-down";
    case APPCOMMAND_DELETE:
      return "delete";
    case APPCOMMAND_DICTATE_OR_COMMAND_CONTROL_TOGGLE:
      return "dictate-or-command-control-toggle";
    default:
      return "unknown";
  }
}

// Copied from ui/views/win/hwnd_message_handler.cc
gfx::ResizeEdge GetWindowResizeEdge(WPARAM param) {
  switch (param) {
    case WMSZ_BOTTOM:
      return gfx::ResizeEdge::kBottom;
    case WMSZ_TOP:
      return gfx::ResizeEdge::kTop;
    case WMSZ_LEFT:
      return gfx::ResizeEdge::kLeft;
    case WMSZ_RIGHT:
      return gfx::ResizeEdge::kRight;
    case WMSZ_TOPLEFT:
      return gfx::ResizeEdge::kTopLeft;
    case WMSZ_TOPRIGHT:
      return gfx::ResizeEdge::kTopRight;
    case WMSZ_BOTTOMLEFT:
      return gfx::ResizeEdge::kBottomLeft;
    case WMSZ_BOTTOMRIGHT:
      return gfx::ResizeEdge::kBottomRight;
    default:
      return gfx::ResizeEdge::kBottomRight;
  }
}

bool IsMutexPresent(const wchar_t* name) {
  base::win::ScopedHandle mutex_holder(::CreateMutex(nullptr, false, name));
  return ::GetLastError() == ERROR_ALREADY_EXISTS;
}

bool IsLibraryLoaded(const wchar_t* name) {
  HMODULE hmodule = nullptr;
  ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, name,
                      &hmodule);
  return hmodule != nullptr;
}

// The official way to get screen reader status is to call:
// SystemParametersInfo(SPI_GETSCREENREADER) && UiaClientsAreListening()
// However it has false positives (for example when user is using touch screens)
// and will cause performance issues in some apps.
bool IsScreenReaderActive() {
  if (IsMutexPresent(L"NarratorRunning"))
    return true;

  static const wchar_t* names[] = {// NVDA
                                   L"nvdaHelperRemote.dll",
                                   // JAWS
                                   L"jhook.dll",
                                   // Window-Eyes
                                   L"gwhk64.dll", L"gwmhook.dll",
                                   // ZoomText
                                   L"AiSquared.Infuser.HookLib.dll"};

  for (auto* name : names) {
    if (IsLibraryLoaded(name))
      return true;
  }

  return false;
}

}  // namespace

HHOOK NativeWindowViews::mouse_hook_ = nullptr;

bool NativeWindowViews::ExecuteWindowsCommand(int command_id) {
  const auto command_name = AppCommandToString(command_id);
  NotifyWindowExecuteAppCommand(command_name);

  return false;
}

bool NativeWindowViews::PreHandleMSG(UINT message,
                                     WPARAM w_param,
                                     LPARAM l_param,
                                     LRESULT* result) {
  NotifyWindowMessage(message, w_param, l_param);

  // Avoid side effects when calling SetWindowPlacement.
  if (is_setting_window_placement_) {
    // Let Chromium handle the WM_NCCALCSIZE message otherwise the window size
    // would be wrong.
    // See https://github.com/electron/electron/issues/22393 for more.
    if (message == WM_NCCALCSIZE)
      return false;
    // Otherwise handle the message with default proc,
    *result = DefWindowProc(GetAcceleratedWidget(), message, w_param, l_param);
    // and tell Chromium to ignore this message.
    return true;
  }

  if (message == taskbar_created_message_) {
    // We need to reset all of our buttons because the taskbar went away.
    taskbar_host_.RestoreThumbarButtons(GetAcceleratedWidget());
    return true;
  }

  switch (message) {
    // Screen readers send WM_GETOBJECT in order to get the accessibility
    // object, so take this opportunity to push Chromium into accessible
    // mode if it isn't already, always say we didn't handle the message
    // because we still want Chromium to handle returning the actual
    // accessibility object.
    case WM_GETOBJECT: {
      if (checked_for_a11y_support_)
        return false;

      const DWORD obj_id = static_cast<DWORD>(l_param);

      if (obj_id != static_cast<DWORD>(OBJID_CLIENT)) {
        return false;
      }

      if (!IsScreenReaderActive()) {
        return false;
      }

      checked_for_a11y_support_ = true;

      auto* const axState = content::BrowserAccessibilityState::GetInstance();
      if (axState && axState->GetAccessibilityMode() != ui::kAXModeComplete) {
        scoped_accessibility_mode_ =
            content::BrowserAccessibilityState::GetInstance()
                ->CreateScopedModeForProcess(ui::kAXModeComplete);
        Browser::Get()->OnAccessibilitySupportChanged();
      }

      return false;
    }
    case WM_GETMINMAXINFO: {
      WINDOWPLACEMENT wp;
      wp.length = sizeof(WINDOWPLACEMENT);

      // We do this to work around a Windows bug, where the minimized Window
      // would report that the closest display to it is not the one that it was
      // previously on (but the leftmost one instead). We restore the position
      // of the window during the restore operation, this way chromium can
      // use the proper display to calculate the scale factor to use.
      if (!last_normal_placement_bounds_.IsEmpty() &&
          (IsVisible() || IsMinimized()) &&
          GetWindowPlacement(GetAcceleratedWidget(), &wp)) {
        wp.rcNormalPosition = last_normal_placement_bounds_.ToRECT();

        // When calling SetWindowPlacement, Chromium would do window messages
        // handling. But since we are already in PreHandleMSG this would cause
        // crash in Chromium under some cases.
        //
        // We work around the crash by prevent Chromium from handling window
        // messages until the SetWindowPlacement call is done.
        //
        // See https://github.com/electron/electron/issues/21614 for more.
        is_setting_window_placement_ = true;
        SetWindowPlacement(GetAcceleratedWidget(), &wp);
        is_setting_window_placement_ = false;

        last_normal_placement_bounds_ = gfx::Rect();
      }

      return false;
    }
    case WM_COMMAND:
      // Handle thumbar button click message.
      if (HIWORD(w_param) == THBN_CLICKED)
        return taskbar_host_.HandleThumbarButtonEvent(LOWORD(w_param));
      return false;
    case WM_SIZING: {
      is_resizing_ = true;
      bool prevent_default = false;
      gfx::Rect bounds = gfx::Rect(*reinterpret_cast<RECT*>(l_param));
      HWND hwnd = GetAcceleratedWidget();
      gfx::Rect dpi_bounds = ScreenToDIPRect(hwnd, bounds);
      NotifyWindowWillResize(dpi_bounds, GetWindowResizeEdge(w_param),
                             &prevent_default);
      if (prevent_default) {
        ::GetWindowRect(hwnd, reinterpret_cast<RECT*>(l_param));
        pending_bounds_change_.reset();
        return true;  // Tells Windows that the Sizing is handled.
      }
      return false;
    }
    case WM_SIZE: {
      // Handle window state change.
      HandleSizeEvent(w_param, l_param);
      return false;
    }
    case WM_EXITSIZEMOVE: {
      if (is_resizing_) {
        NotifyWindowResized();
        is_resizing_ = false;
      }
      if (is_moving_) {
        NotifyWindowMoved();
        is_moving_ = false;
      }

      // If the user dragged or moved the window during one or more
      // calls to window.setBounds(), we want to apply the most recent
      // one once they are done with the move or resize operation.
      if (pending_bounds_change_.has_value()) {
        SetBounds(pending_bounds_change_.value(), false /* animate */);
        pending_bounds_change_.reset();
      }

      return false;
    }
    case WM_MOVING: {
      is_moving_ = true;
      bool prevent_default = false;
      gfx::Rect bounds = gfx::Rect(*reinterpret_cast<RECT*>(l_param));
      HWND hwnd = GetAcceleratedWidget();
      gfx::Rect dpi_bounds = ScreenToDIPRect(hwnd, bounds);
      NotifyWindowWillMove(dpi_bounds, &prevent_default);
      if (!movable_ || prevent_default) {
        ::GetWindowRect(hwnd, reinterpret_cast<RECT*>(l_param));
        pending_bounds_change_.reset();
        return true;  // Tells Windows that the Move is handled. If not true,
                      // frameless windows can be moved using
                      // -webkit-app-region: drag elements.
      }
      return false;
    }
    case WM_QUERYENDSESSION: {
      bool prevent_default = false;
      std::vector<std::string> reasons = EndSessionToStringVec(l_param);
      NotifyWindowQueryEndSession(reasons, &prevent_default);
      // Result should be TRUE by default, otherwise WM_ENDSESSION will not be
      // fired in some cases: More:
      // https://learn.microsoft.com/en-us/windows/win32/rstmgr/guidelines-for-applications
      *result = !prevent_default;
      return prevent_default;
    }
    case WM_ENDSESSION: {
      std::vector<std::string> reasons = EndSessionToStringVec(l_param);
      if (w_param) {
        NotifyWindowEndSession(reasons);
      }
      return false;
    }
    case WM_PARENTNOTIFY: {
      if (LOWORD(w_param) == WM_CREATE) {
        // Because of reasons regarding legacy drivers and stuff, a window that
        // matches the client area is created and used internally by Chromium.
        // This is used when forwarding mouse messages. We only cache the first
        // occurrence (the webview window) because dev tools also cause this
        // message to be sent.
        if (!legacy_window_) {
          legacy_window_ = reinterpret_cast<HWND>(l_param);
        }
      }
      return false;
    }
    case WM_CONTEXTMENU: {
      // We don't want to trigger system-context-menu here if we have a
      // frameless window as it'll already be emitted in
      // ElectronDesktopWindowTreeHostWin::HandleMouseEvent.
      if (has_frame()) {
        bool prevent_default = false;
        NotifyWindowSystemContextMenu(GET_X_LPARAM(l_param),
                                      GET_Y_LPARAM(l_param), &prevent_default);
        return prevent_default;
      }
      return false;
    }
    case WM_SYSCOMMAND: {
      WPARAM cmd = w_param & 0xFFF0;
      // Needed to account for double clicking title bar to maximize.
      if (transparent() && (cmd == SC_MAXIMIZE))
        return true;
      if (cmd == SC_MINIMIZE)
        was_snapped_ = IsSnapped();
      return false;
    }
    case WM_INITMENU: {
      // This is handling the scenario where the menu might get triggered by the
      // user doing "alt + space" resulting in system maximization and restore
      // being used on transparent windows when that does not work.
      if (transparent()) {
        HMENU menu = GetSystemMenu(GetAcceleratedWidget(), false);
        EnableMenuItem(menu, SC_MAXIMIZE,
                       MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        EnableMenuItem(menu, SC_RESTORE,
                       MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        return true;
      }
      return false;
    }
    case WM_DWMCOLORIZATIONCOLORCHANGED: {
      UpdateWindowAccentColor(IsActive());
      return false;
    }
    case WM_SETTINGCHANGE: {
      if (l_param) {
        const wchar_t* setting_name = reinterpret_cast<const wchar_t*>(l_param);
        std::wstring setting_str(setting_name);
        if (setting_str == L"ImmersiveColorSet")
          UpdateWindowAccentColor(IsActive());
      }
      return false;
    }
    default: {
      return false;
    }
  }
}

void NativeWindowViews::HandleSizeEvent(WPARAM w_param, LPARAM l_param) {
  // Here we handle the WM_SIZE event in order to figure out what is the current
  // window state and notify the user accordingly.
  switch (w_param) {
    case SIZE_MAXIMIZED:
    case SIZE_MINIMIZED: {
      WINDOWPLACEMENT wp;
      wp.length = sizeof(WINDOWPLACEMENT);

      if (GetWindowPlacement(GetAcceleratedWidget(), &wp) && !was_snapped_) {
        last_normal_placement_bounds_ = gfx::Rect(wp.rcNormalPosition);
      }

      // Note that SIZE_MAXIMIZED and SIZE_MINIMIZED might be emitted for
      // multiple times for one resize because of the SetWindowPlacement call.
      if (w_param == SIZE_MAXIMIZED &&
          last_window_state_ != ui::mojom::WindowShowState::kMaximized) {
        if (last_window_state_ == ui::mojom::WindowShowState::kMinimized) {
          NotifyWindowRestore();
          if (was_snapped_)
            was_snapped_ = false;
        }
        last_window_state_ = ui::mojom::WindowShowState::kMaximized;
        NotifyWindowMaximize();
        ResetWindowControls();
      } else if (w_param == SIZE_MINIMIZED &&
                 last_window_state_ != ui::mojom::WindowShowState::kMinimized) {
        last_window_state_ = ui::mojom::WindowShowState::kMinimized;
        NotifyWindowMinimize();
      }
      break;
    }
    case SIZE_RESTORED: {
      switch (last_window_state_) {
        case ui::mojom::WindowShowState::kMaximized:
          last_window_state_ = ui::mojom::WindowShowState::kNormal;
          NotifyWindowUnmaximize();
          break;
        case ui::mojom::WindowShowState::kMinimized:
          if (IsFullscreen()) {
            last_window_state_ = ui::mojom::WindowShowState::kFullscreen;
            NotifyWindowEnterFullScreen();
          } else {
            last_window_state_ = ui::mojom::WindowShowState::kNormal;
            NotifyWindowRestore();
            if (was_snapped_)
              was_snapped_ = false;
          }
          break;
        default:
          break;
      }
      ResetWindowControls();
      break;
    }
  }
}

void NativeWindowViews::UpdateWindowAccentColor(bool active) {
  if (base::win::GetVersion() < base::win::Version::WIN11)
    return;

  std::optional<COLORREF> border_color;
  bool should_apply_accent = false;

  if (std::holds_alternative<SkColor>(accent_color_)) {
    // If the user has explicitly set an accent color, use it
    // regardless of whether the system accent color is enabled.
    SkColor color = std::get<SkColor>(accent_color_);
    border_color =
        RGB(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color));
    should_apply_accent = true;
  } else if (std::holds_alternative<bool>(accent_color_)) {
    // Allow the user to optionally force system color on/off.
    should_apply_accent = std::get<bool>(accent_color_);
  } else if (std::holds_alternative<std::monostate>(accent_color_)) {
    // If no explicit color was set, default to the system accent color.
    should_apply_accent = IsAccentColorOnTitleBarsEnabled() && active;
  }

  // Use system accent color as fallback if no explicit color was set.
  if (!border_color.has_value() && should_apply_accent) {
    std::optional<DWORD> system_accent_color = GetSystemAccentColor();
    if (system_accent_color.has_value()) {
      border_color = RGB(GetRValue(system_accent_color.value()),
                         GetGValue(system_accent_color.value()),
                         GetBValue(system_accent_color.value()));
    }
  }

  COLORREF final_color = border_color.value_or(DWMWA_COLOR_DEFAULT);
  SetWindowBorderAndCaptionColor(GetAcceleratedWidget(), final_color,
                                 has_frame());
}

void NativeWindowViews::SetAccentColor(
    std::variant<std::monostate, bool, SkColor> accent_color) {
  accent_color_ = accent_color;
}

/*
 * Returns the window's accent color, per the following heuristic:
 *
 * - If |accent_color_| is an SkColor, return that color as a hex string.
 * - If |accent_color_| is true, return the system accent color as a hex string.
 * - If |accent_color_| is false, return false.
 * - Otherwise, return the system accent color as a hex string.
 */
std::variant<bool, std::string> NativeWindowViews::GetAccentColor() const {
  std::optional<DWORD> system_color = GetSystemAccentColor();

  if (std::holds_alternative<SkColor>(accent_color_)) {
    return ToRGBHex(std::get<SkColor>(accent_color_));
  } else if (std::holds_alternative<bool>(accent_color_)) {
    if (std::get<bool>(accent_color_)) {
      if (!system_color.has_value())
        return false;
      return ToRGBHex(skia::COLORREFToSkColor(system_color.value()));
    } else {
      return false;
    }
  } else {
    if (!system_color.has_value())
      return false;
    return ToRGBHex(skia::COLORREFToSkColor(system_color.value()));
  }
}

void NativeWindowViews::ResetWindowControls() {
  // If a given window was minimized and has since been
  // unminimized (restored/maximized), ensure the WCO buttons
  // are reset to their default unpressed state.
  auto* ncv = widget()->non_client_view();
  if (IsWindowControlsOverlayEnabled() && ncv) {
    auto* frame_view = static_cast<WinFrameView*>(ncv->frame_view());
    frame_view->caption_button_container()->ResetWindowControls();
  }
}

// Windows with |backgroundMaterial| expand to the same dimensions and
// placement as the display to approximate maximization - unless we remove
// rounded corners there will be a gap between the window and the display
// at the corners noticable to users.
void NativeWindowViews::SetRoundedCorners(bool rounded) {
  // DWMWA_WINDOW_CORNER_PREFERENCE is supported after Windows 11 Build 22000.
  if (base::win::GetVersion() < base::win::Version::WIN11)
    return;

  DWM_WINDOW_CORNER_PREFERENCE round_pref =
      rounded ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
  HRESULT result = DwmSetWindowAttribute(GetAcceleratedWidget(),
                                         DWMWA_WINDOW_CORNER_PREFERENCE,
                                         &round_pref, sizeof(round_pref));
  if (FAILED(result))
    LOG(WARNING) << "Failed to set rounded corners to " << rounded;
}

void NativeWindowViews::SetForwardMouseMessages(bool forward) {
  if (forward && !forwarding_mouse_messages_) {
    forwarding_mouse_messages_ = true;
    forwarding_windows_->insert(this);

    // Subclassing is used to fix some issues when forwarding mouse messages;
    // see comments in |SubclassProc|.
    SetWindowSubclass(legacy_window_, SubclassProc, 1,
                      reinterpret_cast<DWORD_PTR>(this));

    if (!mouse_hook_) {
      mouse_hook_ = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, nullptr, 0);
    }
  } else if (!forward && forwarding_mouse_messages_) {
    forwarding_mouse_messages_ = false;
    forwarding_windows_->erase(this);

    RemoveWindowSubclass(legacy_window_, SubclassProc, 1);

    if (forwarding_windows_->empty()) {
      UnhookWindowsHookEx(mouse_hook_);
      mouse_hook_ = nullptr;
    }
  }
}

LRESULT CALLBACK NativeWindowViews::SubclassProc(HWND hwnd,
                                                 UINT msg,
                                                 WPARAM w_param,
                                                 LPARAM l_param,
                                                 UINT_PTR subclass_id,
                                                 DWORD_PTR ref_data) {
  auto* window = reinterpret_cast<NativeWindowViews*>(ref_data);
  switch (msg) {
    case WM_MOUSELEAVE: {
      // When input is forwarded to underlying windows, this message is posted.
      // If not handled, it interferes with Chromium logic, causing for example
      // mouseleave events to fire. If those events are used to exit forward
      // mode, excessive flickering on for example hover items in underlying
      // windows can occur due to rapidly entering and leaving forwarding mode.
      // By consuming and ignoring the message, we're essentially telling
      // Chromium that we have not left the window despite somebody else getting
      // the messages. As to why this is caught for the legacy window and not
      // the actual browser window is simply that the legacy window somehow
      // makes use of these events; posting to the main window didn't work.
      if (window->forwarding_mouse_messages_) {
        return 0;
      }
      break;
    }
  }

  return DefSubclassProc(hwnd, msg, w_param, l_param);
}

LRESULT CALLBACK NativeWindowViews::MouseHookProc(int n_code,
                                                  WPARAM w_param,
                                                  LPARAM l_param) {
  if (n_code < 0) {
    return CallNextHookEx(nullptr, n_code, w_param, l_param);
  }

  // Post a WM_MOUSEMOVE message for those windows whose client area contains
  // the cursor since they are in a state where they would otherwise ignore all
  // mouse input.
  if (w_param == WM_MOUSEMOVE) {
    for (auto* window : *forwarding_windows_) {
      // At first I considered enumerating windows to check whether the cursor
      // was directly above the window, but since nothing bad seems to happen
      // if we post the message even if some other window occludes it I have
      // just left it as is.
      RECT client_rect;
      GetClientRect(window->legacy_window_, &client_rect);
      POINT p = reinterpret_cast<MSLLHOOKSTRUCT*>(l_param)->pt;
      ScreenToClient(window->legacy_window_, &p);
      if (PtInRect(&client_rect, p)) {
        WPARAM w = 0;  // No virtual keys pressed for our purposes
        LPARAM l = MAKELPARAM(p.x, p.y);
        PostMessage(window->legacy_window_, WM_MOUSEMOVE, w, l);
      }
    }
  }

  return CallNextHookEx(nullptr, n_code, w_param, l_param);
}

}  // namespace electron
