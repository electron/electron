// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_MAC_COCOA_NOTIFICATION_H_
#define BRIGHTRAY_BROWSER_MAC_COCOA_NOTIFICATION_H_

#import <Foundation/Foundation.h>

#include <string>
#include <vector>

#include "base/mac/scoped_nsobject.h"
#include "brightray/browser/notification.h"

namespace brightray {

class CocoaNotification : public Notification {
 public:
  CocoaNotification(NotificationDelegate* delegate,
                    NotificationPresenter* presenter);
  ~CocoaNotification();

  // Notification:
  void Show(const NotificationOptions& options) override;
  void Dismiss() override;

  void NotificationDisplayed();
  void NotificationReplied(const std::string& reply);
  void NotificationButtonClicked();

  NSUserNotification* notification() const { return notification_; }

 private:
  void LogAction(const char* action);

  base::scoped_nsobject<NSUserNotification> notification_;
  int action_index_;

  DISALLOW_COPY_AND_ASSIGN(CocoaNotification);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_MAC_COCOA_NOTIFICATION_H_
