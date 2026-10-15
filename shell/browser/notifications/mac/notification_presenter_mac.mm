// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/notifications/mac/notification_presenter_mac.h"

#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "shell/browser/notifications/mac/cocoa_notification.h"
#include "shell/browser/notifications/mac/notification_center_delegate.h"

namespace electron {

// static
std::unique_ptr<NotificationPresenter> NotificationPresenter::Create() {
  return std::make_unique<NotificationPresenterMac>();
}

CocoaNotification* NotificationPresenterMac::GetNotification(
    UNNotificationRequest* un_notification_request) {
  for (Notification* notification : notifications()) {
    auto* native_notification = static_cast<CocoaNotification*>(notification);
    if ([native_notification->notification_request().identifier
            isEqual:un_notification_request.identifier])
      return native_notification;
  }

  if (electron::debug_notifications) {
    LOG(INFO) << "Could not find notification for "
              << [un_notification_request.identifier UTF8String];
  }

  return nullptr;
}

NotificationPresenterMac::NotificationPresenterMac()
    : notification_center_delegate_(
          [[NotificationCenterDelegate alloc] initWithPresenter:this]) {
  UNUserNotificationCenter* center =
      [UNUserNotificationCenter currentNotificationCenter];

  center.delegate = notification_center_delegate_;

  [center
      requestAuthorizationWithOptions:(UNAuthorizationOptionAlert |
                                       UNAuthorizationOptionSound |
                                       UNAuthorizationOptionBadge)
                    completionHandler:^(BOOL granted,
                                        NSError* _Nullable error) {
                      if (electron::debug_notifications) {
                        if (error) {
                          LOG(INFO)
                              << "Error requesting notification authorization: "
                              << [error.localizedDescription UTF8String];
                        } else {
                          LOG(INFO) << "Notification authorization granted: "
                                    << granted;
                        }
                      }
                    }];
}

NotificationPresenterMac::~NotificationPresenterMac() {
  UNUserNotificationCenter.currentNotificationCenter.delegate = nil;
}

Notification* NotificationPresenterMac::CreateNotificationObject(
    NotificationDelegate* delegate) {
  return new CocoaNotification(delegate, this);
}

void NotificationPresenterMac::GetDeliveredNotifications(
    GetDeliveredNotificationsCallback callback) {
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::SequencedTaskRunner::GetCurrentDefault();

  __block GetDeliveredNotificationsCallback block_callback =
      std::move(callback);

  [[UNUserNotificationCenter currentNotificationCenter]
      getDeliveredNotificationsWithCompletionHandler:^(
          NSArray<UNNotification*>* _Nonnull notifications) {
        std::vector<NotificationInfo> results;
        results.reserve([notifications count]);

        for (UNNotification* notification in notifications) {
          UNNotificationContent* content = notification.request.content;
          NotificationInfo info;
          info.id = base::SysNSStringToUTF8(notification.request.identifier);
          info.title = base::SysNSStringToUTF8(content.title);
          info.subtitle = base::SysNSStringToUTF8(content.subtitle);
          info.body = base::SysNSStringToUTF8(content.body);
          info.group_id = base::SysNSStringToUTF8(content.threadIdentifier);
          results.push_back(std::move(info));
        }

        task_runner->PostTask(
            FROM_HERE,
            base::BindOnce(std::move(block_callback), std::move(results)));
      }];
}

void NotificationPresenterMac::RemoveDeliveredNotifications(
    const std::vector<std::string>& identifiers) {
  NSMutableArray* ns_identifiers =
      [NSMutableArray arrayWithCapacity:identifiers.size()];
  for (const auto& id : identifiers) {
    [ns_identifiers addObject:base::SysUTF8ToNSString(id)];
  }
  [[UNUserNotificationCenter currentNotificationCenter]
      removeDeliveredNotificationsWithIdentifiers:ns_identifiers];
}

void NotificationPresenterMac::RemoveAllDeliveredNotifications() {
  [[UNUserNotificationCenter currentNotificationCenter]
      removeAllDeliveredNotifications];
}

void NotificationPresenterMac::RemoveDeliveredNotificationsByGroupId(
    const std::string& group_id) {
  NSString* target_group = base::SysUTF8ToNSString(group_id);
  UNUserNotificationCenter* center =
      [UNUserNotificationCenter currentNotificationCenter];

  [center getDeliveredNotificationsWithCompletionHandler:^(
              NSArray<UNNotification*>* _Nonnull notifications) {
    NSMutableArray* matching_ids = [NSMutableArray array];
    for (UNNotification* notification in notifications) {
      if ([notification.request.content.threadIdentifier
              isEqualToString:target_group]) {
        [matching_ids addObject:notification.request.identifier];
      }
    }
    if (matching_ids.count > 0) {
      [center removeDeliveredNotificationsWithIdentifiers:matching_ids];
    }
  }];
}

}  // namespace electron
