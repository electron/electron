// Copyright (c) 2022 Asana, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/electron_main_delegate.h"
#include "shell/browser/api/electron_api_push_notifications.h"

#include <string>

#import "shell/browser/mac/electron_application.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

void PushNotifications::RegisterForAPNSNotifications() {
  [[AtomApplication sharedApplication]
      registerForRemoteNotificationTypes:NSRemoteNotificationTypeBadge |
                                         NSRemoteNotificationTypeAlert |
                                         NSRemoteNotificationTypeSound];
}

void PushNotifications::UnregisterForAPNSNotifications() {
  [[AtomApplication sharedApplication] unregisterForRemoteNotifications];
}

void PushNotifications::OnDidRegisterForAPNSNotificationsWithDeviceToken(
    const std::string& token) {
  Emit("registered-for-apns-notifications", token);
}

void PushNotifications::OnDidFailToRegisterForAPNSNotificationsWithError(
    const std::string& error) {
  Emit("failed-to-register-for-apns-notifications", error);
}

void PushNotifications::OnDidReceiveAPNSNotification(
    const base::DictionaryValue& user_info) {
  Emit("received-apns-notification", user_info);
}

}  // namespace api

}  // namespace electron
