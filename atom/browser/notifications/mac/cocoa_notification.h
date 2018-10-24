// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_
#define ATOM_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_

#import <Foundation/Foundation.h>

#include <map>
#include <string>
#include <vector>

#include "atom/browser/notifications/notification.h"
#include "base/mac/scoped_nsobject.h"

namespace atom {

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

  base::scoped_nsobject<NSUserNotification> notification_;
  std::map<std::string, unsigned> additional_action_indices_;
  unsigned action_index_;

  DISALLOW_COPY_AND_ASSIGN(CocoaNotification);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_
