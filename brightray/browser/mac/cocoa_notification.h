// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BROWSER_MAC_COCOA_NOTIFICATION_H_
#define BROWSER_MAC_COCOA_NOTIFICATION_H_

#import <Foundation/Foundation.h>

#include "base/mac/scoped_nsobject.h"
#include "base/memory/scoped_ptr.h"
#include "browser/notification.h"

namespace brightray {

class CocoaNotification : public Notification {
 public:
  CocoaNotification(NotificationDelegate* delegate,
                    NotificationPresenter* presenter);
  ~CocoaNotification();

  // Notification:
  void Show(const base::string16& title,
            const base::string16& msg,
            const std::string& tag,
            const GURL& icon_url,
            const SkBitmap& icon,
            const bool silent) override;
  void Dismiss() override;

  void NotificationDisplayed();

  NSUserNotification* notification() const { return notification_; }

 private:
  base::scoped_nsobject<NSUserNotification> notification_;

  DISALLOW_COPY_AND_ASSIGN(CocoaNotification);
};

}  // namespace brightray

#endif  // BROWSER_MAC_COCOA_NOTIFICATION_H_
