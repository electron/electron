// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_WIN_NOTIFY_ICON_HOST_H_
#define SHELL_BROWSER_UI_WIN_NOTIFY_ICON_HOST_H_

#include <windows.h>

#include <vector>

#include "base/macros.h"
#include "base/optional.h"
#include "shell/common/gin_converters/guid_converter.h"

const GUID GUID_DEFAULT = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

namespace electron {

class NotifyIcon;

class NotifyIconHost {
 public:
  NotifyIconHost();
  ~NotifyIconHost();

  NotifyIcon* CreateNotifyIcon(base::Optional<UUID> guid);
  void Remove(NotifyIcon* notify_icon);

 private:
  typedef std::vector<NotifyIcon*> NotifyIcons;

  // Static callback invoked when a message comes in to our messaging window.
  static LRESULT CALLBACK WndProcStatic(HWND hwnd,
                                        UINT message,
                                        WPARAM wparam,
                                        LPARAM lparam);

  LRESULT CALLBACK WndProc(HWND hwnd,
                           UINT message,
                           WPARAM wparam,
                           LPARAM lparam);

  UINT NextIconId();

  // The unique icon ID we will assign to the next icon.
  UINT next_icon_id_ = 1;

  // List containing all active NotifyIcons.
  NotifyIcons notify_icons_;

  // The window class of |window_|.
  ATOM atom_ = 0;

  // The handle of the module that contains the window procedure of |window_|.
  HMODULE instance_ = nullptr;

  // The window used for processing events.
  HWND window_ = nullptr;

  // The message ID of the "TaskbarCreated" message, sent to us when we need to
  // reset our status icons.
  UINT taskbar_created_message_ = 0;

  DISALLOW_COPY_AND_ASSIGN(NotifyIconHost);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_WIN_NOTIFY_ICON_HOST_H_
