// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_NOTIFICATION_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_NOTIFICATION_H_

#include <string>

#include "shell/browser/notifications/notification.h"
#include "shell/browser/notifications/win/notification_presenter_win7.h"

namespace electron {

class Win32Notification : public electron::Notification {
 public:
  Win32Notification(NotificationDelegate* delegate,
                    NotificationPresenterWin7* presenter)
      : Notification(delegate, presenter) {}
  void Show(const NotificationOptions& options) override;
  void Dismiss() override;

  const DesktopNotificationController::Notification& GetRef() const {
    return notification_ref_;
  }

  const std::string& GetTag() const { return tag_; }

 private:
  DesktopNotificationController::Notification notification_ref_;
  std::string tag_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_NOTIFICATION_H_
