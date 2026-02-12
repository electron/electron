// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/task/thread_pool.h"

#include "shell/browser/notifications/mac/notification_presenter_mac.h"

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
          [[NotificationCenterDelegate alloc] initWithPresenter:this]),
      image_retainer_(std::make_unique<NotificationImageRetainer>()),
      image_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE})) {
  // Delete any remaining temp files in the image folder from the previous
  // sessions.
  DCHECK(image_task_runner_);
  auto cleanup_task = image_retainer_->GetCleanupTask();
  image_task_runner_->PostTask(FROM_HERE, std::move(cleanup_task));

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
  image_task_runner_->DeleteSoon(FROM_HERE, image_retainer_.release());
}

Notification* NotificationPresenterMac::CreateNotificationObject(
    NotificationDelegate* delegate) {
  return new CocoaNotification(delegate, this);
}

}  // namespace electron
