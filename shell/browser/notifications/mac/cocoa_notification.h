// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>

#include <map>
#include <string>

#include "shell/browser/notifications/notification.h"

namespace electron {

class CocoaNotification : public Notification {
 public:
  CocoaNotification(NotificationDelegate* delegate,
                    NotificationPresenter* presenter);
  ~CocoaNotification() override;

  // Notification:
  void Show(const NotificationOptions& options) override;
  void Dismiss() override;

  void NotificationDisplayed();
  void NotificationReplied(const std::string& reply);
  void NotificationActivated(int actionIndex);
  void NotificationDismissed();

  UNNotificationRequest* notification_request() const {
    return notification_request_;
  }

 private:
  void LogAction(const char* action);

  UNNotificationRequest* __strong notification_request_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_
