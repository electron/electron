// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN7_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN7_H_

#include <string>

#include "shell/browser/notifications/notification_presenter.h"
#include "shell/browser/notifications/win/win32_desktop_notifications/desktop_notification_controller.h"

namespace electron {

class Win32Notification;

class NotificationPresenterWin7 : public NotificationPresenter,
                                  public DesktopNotificationController {
 public:
  NotificationPresenterWin7() = default;

  Win32Notification* GetNotificationObjectByRef(
      const DesktopNotificationController::Notification& ref);

  Win32Notification* GetNotificationObjectByTag(const std::string& tag);

 private:
  electron::Notification* CreateNotificationObject(
      NotificationDelegate* delegate) override;

  void OnNotificationClicked(const Notification& notification) override;
  void OnNotificationDismissed(const Notification& notification) override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN7_H_
