// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN7_H_
#define ATOM_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN7_H_

#include <string>

#include "atom/browser/notifications/notification_presenter.h"
#include "atom/browser/notifications/win/win32_desktop_notifications/desktop_notification_controller.h"

namespace atom {

class Win32Notification;

class NotificationPresenterWin7 : public NotificationPresenter,
                                  public DesktopNotificationController {
 public:
  NotificationPresenterWin7() = default;

  Win32Notification* GetNotificationObjectByRef(
      const DesktopNotificationController::Notification& ref);

  Win32Notification* GetNotificationObjectByTag(const std::string& tag);

 private:
  atom::Notification* CreateNotificationObject(
      NotificationDelegate* delegate) override;

  void OnNotificationClicked(const Notification& notification) override;
  void OnNotificationDismissed(const Notification& notification) override;

  DISALLOW_COPY_AND_ASSIGN(NotificationPresenterWin7);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN7_H_
