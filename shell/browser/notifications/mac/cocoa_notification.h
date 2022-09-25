// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_

#import <Foundation/Foundation.h>

#include <map>
#include <string>

#include "base/mac/scoped_nsobject.h"
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
  void Destroy() override;

  void NotificationDisplayed();
  void NotificationClicked();
  void NotificationReplied(const std::string& reply);
  void NotificationActivated();
  void NotificationActivated(NSUserNotificationAction* action);
  void NotificationDismissed();

  NSUserNotification* notification() const { return notification_; }

 private:
  void LogAction(const char* action);
  // SAP-14036 upgrade for persistent notifications support
  bool is_persistent_;
  bool allow_destroy_;

  base::scoped_nsobject<NSUserNotification> notification_;
  std::map<std::string, unsigned> additional_action_indices_;
  void deliverXPCNotification(NSUserNotification* notification);
  void removeXPCNotification(NSUserNotification* notification);
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_
