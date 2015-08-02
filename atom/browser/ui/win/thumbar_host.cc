// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/thumbar_host.h"

#include <shobjidl.h>

#include "base/win/scoped_comptr.h"
#include "base/win/win_util.h"
#include "base/win/wrapped_window_proc.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/icon_util.h"
#include "ui/gfx/win/hwnd_util.h"

namespace atom {

namespace {

// The thumbnail toolbar has a maximum of seven buttons due to the limited room.
const int kMaxButtonsCount = 7;

bool GetThumbarButtonFlags(const std::vector<std::string>& flags,
                           THUMBBUTTONFLAGS* out) {
  if (flags.empty()) {
    *out = THBF_ENABLED;
    return true;
  }
  THUMBBUTTONFLAGS result = static_cast<THUMBBUTTONFLAGS>(0);
  for (const auto& flag : flags) {
    if (flag == "enabled") {
      result |= THBF_ENABLED;
    } else if (flag == "disabled") {
      result |= THBF_DISABLED;
    } else if (flag == "dismissonclick") {
      result |= THBF_DISMISSONCLICK;
    } else if (flag == "nobackground") {
      result |= THBF_NOBACKGROUND;
    } else if (flag == "hidden") {
      result |= THBF_HIDDEN;
    } else if (flag == "noninteractive") {
      result |= THBF_NONINTERACTIVE;
    } else {
      return false;
    }
  }
  *out = result;
  return true;
}

}  // namespace

ThumbarHost::ThumbarHost(HWND window) : is_initialized_(false),
                                        window_(window) {
}

ThumbarHost::~ThumbarHost() {
}

bool ThumbarHost::SetThumbarButtons(
    const std::vector<ThumbarHost::ThumbarButton>& buttons) {
  THUMBBUTTON thumb_buttons[kMaxButtonsCount];
  thumbar_button_clicked_callback_map_.clear();

  // Once a toolbar with a set of buttons is added to thumbnail, there is no way
  // to remove it without re-creating the window.
  // To achieve to re-set thumbar buttons,  we initialize the buttons with
  // HIDDEN state and only updated the caller's specified buttons.
  //
  // Initialize all thumb buttons with HIDDEN state.
  for (int i = 0; i < kMaxButtonsCount; ++i) {
    thumb_buttons[i].iId = i;
    thumb_buttons[i].dwFlags = THBF_HIDDEN;
  }

  // Update the callers' specified buttons.
  for (size_t i = 0; i < buttons.size(); ++i) {
    if (!GetThumbarButtonFlags(buttons[i].flags, &thumb_buttons[i].dwFlags))
      return false;
    thumb_buttons[i].dwMask = THB_ICON | THB_FLAGS;
    thumb_buttons[i].hIcon = IconUtil::CreateHICONFromSkBitmap(
        buttons[i].icon.AsBitmap());
    if (!buttons[i].tooltip.empty()) {
      thumb_buttons[i].dwMask |= THB_TOOLTIP;
      wcscpy_s(thumb_buttons[i].szTip,
               base::UTF8ToUTF16(buttons[i].tooltip).c_str());
    }

    thumbar_button_clicked_callback_map_[i] =
        buttons[i].clicked_callback;
  }

  base::win::ScopedComPtr<ITaskbarList3> taskbar;
  if (FAILED(taskbar.CreateInstance(CLSID_TaskbarList,
                                    nullptr,
                                    CLSCTX_INPROC_SERVER)) ||
      FAILED(taskbar->HrInit())) {
    return false;
  }
  if (!is_initialized_) {
    is_initialized_ = true;
    return taskbar->ThumbBarAddButtons(
        window_, kMaxButtonsCount, thumb_buttons) == S_OK;
  }

  return taskbar->ThumbBarUpdateButtons(
      window_, kMaxButtonsCount, thumb_buttons) == S_OK;
}

bool ThumbarHost::HandleThumbarButtonEvent(int button_id) {
  if (thumbar_button_clicked_callback_map_.find(button_id) !=
      thumbar_button_clicked_callback_map_.end()) {
    auto callback = thumbar_button_clicked_callback_map_[button_id];
    if (!callback.is_null())
      callback.Run();
    return true;
  }
  return false;
}

}  // namespace atom
