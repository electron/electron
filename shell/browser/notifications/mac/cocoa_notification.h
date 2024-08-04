// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_

#import <Foundation/Foundation.h>

#include <map>
#include <string>

#include "shell/browser/notifications/notification.h"

namespace electron {

// NSUserNotification is deprecated; all calls should be replaced with
// UserNotifications.frameworks API
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

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
  void NotificationActivated();
  void NotificationActivated(NSUserNotificationAction* action);
  void NotificationDismissed();

  NSUserNotification* notification() const { return notification_; }

 private:
  void LogAction(const char* action);

  NSUserNotification* __strong notification_;
  std::map<std::string, unsigned> additional_action_indices_;
  unsigned action_index_;
};

// -Wdeprecated-declarations
#pragma clang diagnostic pop

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_
