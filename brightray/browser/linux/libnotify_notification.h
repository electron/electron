// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BROWSER_LINUX_LIBNOTIFY_NOTIFICATION_H_
#define BROWSER_LINUX_LIBNOTIFY_NOTIFICATION_H_

#include "browser/linux/libnotify_loader.h"
#include "browser/notification.h"
#include "ui/base/glib/glib_signal.h"

namespace brightray {

class LibnotifyNotification : public Notification {
 public:
  LibnotifyNotification(NotificationDelegate* delegate,
                        NotificationPresenter* presenter);
  virtual ~LibnotifyNotification();

  static bool Initialize();

  // Notification:
  void Show(const base::string16& title,
            const base::string16& msg,
            const GURL& icon_url,
            const SkBitmap& icon,
            const bool silent) override;
  void Dismiss() override;

 private:
  CHROMEG_CALLBACK_0(LibnotifyNotification, void, OnNotificationClosed,
                     NotifyNotification*);
  CHROMEG_CALLBACK_1(LibnotifyNotification, void, OnNotificationView,
                     NotifyNotification*, char*);

  void NotificationFailed();

  NotifyNotification* notification_;

  DISALLOW_COPY_AND_ASSIGN(LibnotifyNotification);
};

}  // namespace brightray

#endif  // BROWSER_LINUX_LIBNOTIFY_NOTIFICATION_H_
