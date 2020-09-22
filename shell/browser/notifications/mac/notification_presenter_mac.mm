// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/logging.h"

#include "shell/browser/notifications/mac/notification_presenter_mac.h"

#include "shell/browser/notifications/mac/cocoa_notification.h"
#include "shell/browser/notifications/mac/notification_center_delegate.h"

namespace electron {

// static
NotificationPresenter* NotificationPresenter::Create() {
  return new NotificationPresenterMac;
}

CocoaNotification* NotificationPresenterMac::GetNotification(
    NSUserNotification* ns_notification) {
  for (Notification* notification : notifications()) {
    auto* native_notification = static_cast<CocoaNotification*>(notification);
    if ([native_notification->notification().identifier
            isEqual:ns_notification.identifier])
      return native_notification;
  }

  if (getenv("ELECTRON_DEBUG_NOTIFICATIONS")) {
    LOG(INFO) << "Could not find notification for "
              << [ns_notification.identifier UTF8String];
  }

  return nullptr;
}

NotificationPresenterMac::NotificationPresenterMac()
    : notification_center_delegate_(
          [[NotificationCenterDelegate alloc] initWithPresenter:this]) {
  NSUserNotificationCenter.defaultUserNotificationCenter.delegate =
      notification_center_delegate_;
}

NotificationPresenterMac::~NotificationPresenterMac() {
  NSUserNotificationCenter.defaultUserNotificationCenter.delegate = nil;
}

Notification* NotificationPresenterMac::CreateNotificationObject(
    NotificationDelegate* delegate) {
  return new CocoaNotification(delegate, this);
}

}  // namespace electron
