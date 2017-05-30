// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_LINUX_LIBNOTIFY_NOTIFICATION_H_
#define BRIGHTRAY_BROWSER_LINUX_LIBNOTIFY_NOTIFICATION_H_

#include <string>

#include "brightray/browser/linux/libnotify_loader.h"
#include "brightray/browser/notification.h"
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
            const std::string& tag,
            const GURL& icon_url,
            const SkBitmap& icon,
            bool silent,
            bool has_reply,
            const base::string16& reply_placeholder) override;
  void Dismiss() override;

 private:
  CHROMEG_CALLBACK_0(LibnotifyNotification, void, OnNotificationClosed,
                     NotifyNotification*);
  CHROMEG_CALLBACK_1(LibnotifyNotification, void, OnNotificationView,
                     NotifyNotification*, char*);

  NotifyNotification* notification_;

  DISALLOW_COPY_AND_ASSIGN(LibnotifyNotification);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_LINUX_LIBNOTIFY_NOTIFICATION_H_
