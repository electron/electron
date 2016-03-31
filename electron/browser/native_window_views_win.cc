// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window_views.h"
#include "content/public/browser/browser_accessibility_state.h"

namespace atom {

namespace {

// Convert Win32 WM_APPCOMMANDS to strings.
const char* AppCommandToString(int command_id) {
  switch (command_id) {
    case APPCOMMAND_BROWSER_BACKWARD       : return "browser-backward";
    case APPCOMMAND_BROWSER_FORWARD        : return "browser-forward";
    case APPCOMMAND_BROWSER_REFRESH        : return "browser-refresh";
    case APPCOMMAND_BROWSER_STOP           : return "browser-stop";
    case APPCOMMAND_BROWSER_SEARCH         : return "browser-search";
    case APPCOMMAND_BROWSER_FAVORITES      : return "browser-favorites";
    case APPCOMMAND_BROWSER_HOME           : return "browser-home";
    case APPCOMMAND_VOLUME_MUTE            : return "volume-mute";
    case APPCOMMAND_VOLUME_DOWN            : return "volume-down";
    case APPCOMMAND_VOLUME_UP              : return "volume-up";
    case APPCOMMAND_MEDIA_NEXTTRACK        : return "media-nexttrack";
    case APPCOMMAND_MEDIA_PREVIOUSTRACK    : return "media-previoustrack";
    case APPCOMMAND_MEDIA_STOP             : return "media-stop";
    case APPCOMMAND_MEDIA_PLAY_PAUSE       : return "media-play_pause";
    case APPCOMMAND_LAUNCH_MAIL            : return "launch-mail";
    case APPCOMMAND_LAUNCH_MEDIA_SELECT    : return "launch-media-select";
    case APPCOMMAND_LAUNCH_APP1            : return "launch-app1";
    case APPCOMMAND_LAUNCH_APP2            : return "launch-app2";
    case APPCOMMAND_BASS_DOWN              : return "bass-down";
    case APPCOMMAND_BASS_BOOST             : return "bass-boost";
    case APPCOMMAND_BASS_UP                : return "bass-up";
    case APPCOMMAND_TREBLE_DOWN            : return "treble-down";
    case APPCOMMAND_TREBLE_UP              : return "treble-up";
    case APPCOMMAND_MICROPHONE_VOLUME_MUTE : return "microphone-volume-mute";
    case APPCOMMAND_MICROPHONE_VOLUME_DOWN : return "microphone-volume-down";
    case APPCOMMAND_MICROPHONE_VOLUME_UP   : return "microphone-volume-up";
    case APPCOMMAND_HELP                   : return "help";
    case APPCOMMAND_FIND                   : return "find";
    case APPCOMMAND_NEW                    : return "new";
    case APPCOMMAND_OPEN                   : return "open";
    case APPCOMMAND_CLOSE                  : return "close";
    case APPCOMMAND_SAVE                   : return "save";
    case APPCOMMAND_PRINT                  : return "print";
    case APPCOMMAND_UNDO                   : return "undo";
    case APPCOMMAND_REDO                   : return "redo";
    case APPCOMMAND_COPY                   : return "copy";
    case APPCOMMAND_CUT                    : return "cut";
    case APPCOMMAND_PASTE                  : return "paste";
    case APPCOMMAND_REPLY_TO_MAIL          : return "reply-to-mail";
    case APPCOMMAND_FORWARD_MAIL           : return "forward-mail";
    case APPCOMMAND_SEND_MAIL              : return "send-mail";
    case APPCOMMAND_SPELL_CHECK            : return "spell-check";
    case APPCOMMAND_MIC_ON_OFF_TOGGLE      : return "mic-on-off-toggle";
    case APPCOMMAND_CORRECTION_LIST        : return "correction-list";
    case APPCOMMAND_MEDIA_PLAY             : return "media-play";
    case APPCOMMAND_MEDIA_PAUSE            : return "media-pause";
    case APPCOMMAND_MEDIA_RECORD           : return "media-record";
    case APPCOMMAND_MEDIA_FAST_FORWARD     : return "media-fast-forward";
    case APPCOMMAND_MEDIA_REWIND           : return "media-rewind";
    case APPCOMMAND_MEDIA_CHANNEL_UP       : return "media-channel-up";
    case APPCOMMAND_MEDIA_CHANNEL_DOWN     : return "media-channel-down";
    case APPCOMMAND_DELETE                 : return "delete";
    case APPCOMMAND_DICTATE_OR_COMMAND_CONTROL_TOGGLE:
      return "dictate-or-command-control-toggle";
    default:
      return "unknown";
  }
}

}  // namespace

bool NativeWindowViews::ExecuteWindowsCommand(int command_id) {
  std::string command = AppCommandToString(command_id);
  NotifyWindowExecuteWindowsCommand(command);
  return false;
}

bool NativeWindowViews::PreHandleMSG(
    UINT message, WPARAM w_param, LPARAM l_param, LRESULT* result) {
  NotifyWindowMessage(message, w_param, l_param);

  switch (message) {
    case WM_COMMAND:
      // Handle thumbar button click message.
      if (HIWORD(w_param) == THBN_CLICKED)
        return taskbar_host_.HandleThumbarButtonEvent(LOWORD(w_param));
      return false;

    case WM_SIZE:
      // Handle window state change.
      HandleSizeEvent(w_param, l_param);
      return false;

    case WM_MOVING: {
      if (!movable_)
        ::GetWindowRect(GetAcceleratedWidget(), (LPRECT)l_param);
      return false;
    }

    default:
      return false;
  }
}

void NativeWindowViews::HandleSizeEvent(WPARAM w_param, LPARAM l_param) {
  // Here we handle the WM_SIZE event in order to figure out what is the current
  // window state and notify the user accordingly.
  switch (w_param) {
    case SIZE_MAXIMIZED:
      last_window_state_ = ui::SHOW_STATE_MAXIMIZED;
      NotifyWindowMaximize();
      break;
    case SIZE_MINIMIZED:
      last_window_state_ = ui::SHOW_STATE_MINIMIZED;
      NotifyWindowMinimize();
      break;
    case SIZE_RESTORED:
      if (last_window_state_ == ui::SHOW_STATE_NORMAL) {
        // Window was resized so we save it's new size.
        last_normal_size_ = GetSize();
      } else {
        switch (last_window_state_) {
          case ui::SHOW_STATE_MAXIMIZED:
            last_window_state_ = ui::SHOW_STATE_NORMAL;

            // When the window is restored we resize it to the previous known
            // normal size.
            NativeWindow::SetSize(last_normal_size_);

            NotifyWindowUnmaximize();
            break;
          case ui::SHOW_STATE_MINIMIZED:
            if (IsFullscreen()) {
              last_window_state_ = ui::SHOW_STATE_FULLSCREEN;
              NotifyWindowEnterFullScreen();
            } else {
              last_window_state_ = ui::SHOW_STATE_NORMAL;

              // When the window is restored we resize it to the previous known
              // normal size.
              NativeWindow::SetSize(last_normal_size_);

              NotifyWindowRestore();
            }
            break;
        }
      }
      break;
  }
}

}  // namespace atom
