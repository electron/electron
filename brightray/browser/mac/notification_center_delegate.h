// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BROWSER_MAC_NOTIFICATION_DELEGATE_H_
#define BROWSER_MAC_NOTIFICATION_DELEGATE_H_

#import <Foundation/Foundation.h>

namespace brightray {
class NotificationPresenterMac;
}

@interface NotificationCenterDelegate
    : NSObject <NSUserNotificationCenterDelegate> {
 @private
  brightray::NotificationPresenterMac* presenter_;
}
- (instancetype)initWithPresenter:
    (brightray::NotificationPresenterMac*)presenter;
@end

#endif  // BROWSER_MAC_NOTIFICATION_DELEGATE_H_
