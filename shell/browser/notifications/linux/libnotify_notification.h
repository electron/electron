// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_LINUX_LIBNOTIFY_NOTIFICATION_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_LINUX_LIBNOTIFY_NOTIFICATION_H_

#include "library_loaders/libnotify_loader.h"
#include "shell/browser/notifications/notification.h"
#include "ui/base/glib/glib_signal.h"

namespace electron {

class LibnotifyNotification : public Notification {
 public:
  LibnotifyNotification(NotificationDelegate* delegate,
                        NotificationPresenter* presenter);
  ~LibnotifyNotification() override;

  static bool Initialize();

  // Notification:
  void Show(const NotificationOptions& options) override;
  void Dismiss() override;

 private:
  CHROMEG_CALLBACK_0(LibnotifyNotification,
                     void,
                     OnNotificationClosed,
                     NotifyNotification*);
  CHROMEG_CALLBACK_1(LibnotifyNotification,
                     void,
                     OnNotificationView,
                     NotifyNotification*,
                     char*);

  NotifyNotification* notification_ = nullptr;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_LINUX_LIBNOTIFY_NOTIFICATION_H_
