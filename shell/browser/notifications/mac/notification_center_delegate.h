// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_CENTER_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_CENTER_DELEGATE_H_

#import <Foundation/Foundation.h>

#include "base/memory/raw_ptr.h"

namespace electron {
class NotificationPresenterMac;
}

@interface NotificationCenterDelegate
    : NSObject <NSUserNotificationCenterDelegate> {
 @private
  raw_ptr<electron::NotificationPresenterMac> presenter_;
}
- (instancetype)initWithPresenter:
    (electron::NotificationPresenterMac*)presenter;
@end

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_CENTER_DELEGATE_H_
