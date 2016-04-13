// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "browser/mac/cocoa_notification.h"

#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "browser/notification_delegate.h"
#include "browser/notification_presenter.h"
#include "skia/ext/skia_utils_mac.h"

namespace brightray {

// static
Notification* Notification::Create(NotificationDelegate* delegate,
                                   NotificationPresenter* presenter) {
  return new CocoaNotification(delegate, presenter);
}

CocoaNotification::CocoaNotification(NotificationDelegate* delegate,
                                     NotificationPresenter* presenter)
    : Notification(delegate, presenter) {
}

CocoaNotification::~CocoaNotification() {
  [NSUserNotificationCenter.defaultUserNotificationCenter
      removeDeliveredNotification:notification_];
}

void CocoaNotification::Show(const base::string16& title,
                             const base::string16& body,
                             const std::string& tag,
                             const GURL& icon_url,
                             const SkBitmap& icon,
                             const bool silent) {
  notification_.reset([[NSUserNotification alloc] init]);
  [notification_ setTitle:base::SysUTF16ToNSString(title)];
  [notification_ setInformativeText:base::SysUTF16ToNSString(body)];

  if ([notification_ respondsToSelector:@selector(setContentImage:)] &&
      !icon.drawsNothing()) {
    NSImage* image = skia::SkBitmapToNSImageWithColorSpace(
        icon, base::mac::GetGenericRGBColorSpace());
    [notification_ setContentImage:image];
  }

  if (silent) {
    [notification_ setSoundName:nil];
  } else {
    [notification_ setSoundName:NSUserNotificationDefaultSoundName];
  }

  [NSUserNotificationCenter.defaultUserNotificationCenter
      deliverNotification:notification_];
}

void CocoaNotification::Dismiss() {
  [NSUserNotificationCenter.defaultUserNotificationCenter
      removeDeliveredNotification:notification_];
  delegate()->NotificationClosed();
  Destroy();
}

void CocoaNotification::NotifyDisplayed() {
  delegate()->NotificationDisplayed();
}

void CocoaNotification::NotifyClick() {
  delegate()->NotificationClick();
  Destroy();
}

}  // namespace brightray
