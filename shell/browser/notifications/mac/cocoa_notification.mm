// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/notifications/mac/cocoa_notification.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "grit/electron_resources.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/notification_presenter.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/l10n/l10n_util_mac.h"

#import <AppKit/AppKit.h>

namespace electron {

CocoaNotification::CocoaNotification(NotificationDelegate* delegate,
                                     NotificationPresenter* presenter)
    : Notification(delegate, presenter) {}

CocoaNotification::~CocoaNotification() {
  if (notification_request_)
    [[UNUserNotificationCenter currentNotificationCenter]
        removeDeliveredNotificationsWithIdentifiers:@[
          notification_request_.identifier
        ]];
}

void CocoaNotification::Show(const NotificationOptions& options) {
  UNMutableNotificationContent* content =
      [[UNMutableNotificationContent alloc] init];

  content.title = base::SysUTF16ToNSString(options.title);
  content.subtitle = base::SysUTF16ToNSString(options.subtitle);
  content.body = base::SysUTF16ToNSString(options.msg);

  if (!options.icon.drawsNothing()) {
    NSImage* image = skia::SkBitmapToNSImage(options.icon);
    NSData* imageData =
        [[NSBitmapImageRep imageRepWithData:[image TIFFRepresentation]]
            representationUsingType:NSBitmapImageFileTypePNG
                         properties:@{}];
    NSString* tempFilePath = [NSTemporaryDirectory()
        stringByAppendingPathComponent:[NSString
                                           stringWithFormat:@"%@_icon.png",
                                                            [[NSUUID UUID]
                                                                UUIDString]]];
    if ([imageData writeToFile:tempFilePath atomically:YES]) {
      NSError* error = nil;
      UNNotificationAttachment* attachment = [UNNotificationAttachment
          attachmentWithIdentifier:@"image"
                               URL:[NSURL fileURLWithPath:tempFilePath]
                           options:nil
                             error:&error];
      if (attachment && !error) {
        content.attachments = @[ attachment ];
      }
    }
  }

  if (options.silent) {
    content.sound = nil;
  } else if (options.sound.empty()) {
    content.sound = [UNNotificationSound defaultSound];
  } else {
    content.sound = [UNNotificationSound
        soundNamed:base::SysUTF16ToNSString(options.sound)];
  }

  if (options.has_reply || options.actions.size() > 0 ||
      !options.close_button_text.empty()) {
    NSMutableArray* actions = [NSMutableArray array];
    NSMutableString* actionString = [NSMutableString string];

    if (options.has_reply) {
      NSString* replyTitle =
          l10n_util::GetNSString(IDS_MAC_NOTIFICATION_INLINE_REPLY_BUTTON);
      NSString* replyPlaceholder =
          base::SysUTF16ToNSString(options.reply_placeholder);
      UNNotificationAction* replyAction = [UNTextInputNotificationAction
          actionWithIdentifier:@"REPLY_ACTION"
                         title:replyTitle
                       options:UNNotificationActionOptionNone
          textInputButtonTitle:replyTitle
          textInputPlaceholder:replyPlaceholder];
      [actionString appendFormat:@"REPLY_%@_%@", replyTitle, replyPlaceholder];
      [actions addObject:replyAction];
    }

    int i = 0;
    for (const auto& action : options.actions) {
      NSString* showText =
          l10n_util::GetNSString(IDS_MAC_NOTIFICATION_SHOW_BUTTON);
      NSString* actionText = action.text.empty()
                                 ? showText
                                 : base::SysUTF16ToNSString(action.text);
      // Action indicies are stored in the action identifier
      UNNotificationAction* notificationAction = [UNNotificationAction
          actionWithIdentifier:[NSString stringWithFormat:@"ACTION_%d", i]
                         title:actionText
                       options:UNNotificationActionOptionNone];
      [actionString appendFormat:@"ACTION_%d_%@", i, actionText];
      [actions addObject:notificationAction];
      i++;
    }

    if (!options.close_button_text.empty()) {
      NSString* closeButtonText =
          base::SysUTF16ToNSString(options.close_button_text);
      UNNotificationAction* closeAction = [UNNotificationAction
          actionWithIdentifier:@"CLOSE_ACTION"
                         title:closeButtonText
                       options:UNNotificationActionOptionNone];
      [actionString appendFormat:@"CLOSE_%@", closeButtonText];
      [actions addObject:closeAction];
      i++;
    }

    // Categories are unique based on the actionString
    NSString* categoryIdentifier =
        [NSString stringWithFormat:@"CATEGORY_%lu", [actionString hash]];

    // UNNotificationCategoryOptionCustomDismissAction enables the notification
    // delegate to receive dismiss actions for handling
    UNNotificationCategory* category = [UNNotificationCategory
        categoryWithIdentifier:categoryIdentifier
                       actions:actions
             intentIdentifiers:@[]
                       options:UNNotificationCategoryOptionCustomDismissAction];

    UNUserNotificationCenter* center =
        [UNUserNotificationCenter currentNotificationCenter];
    [center getNotificationCategoriesWithCompletionHandler:^(
                NSSet<UNNotificationCategory*>* _Nonnull existingCategories) {
      if (![existingCategories containsObject:category]) {
        NSMutableSet* updatedCategories = [existingCategories mutableCopy];
        [updatedCategories addObject:category];
        [center setNotificationCategories:updatedCategories];
      }
    }];
    content.categoryIdentifier = categoryIdentifier;
  }

  NSString* identifier =
      [NSString stringWithFormat:@"%@:notification:%@",
                                 [[NSBundle mainBundle] bundleIdentifier],
                                 [[NSUUID UUID] UUIDString]];

  UNNotificationRequest* request =
      [UNNotificationRequest requestWithIdentifier:identifier
                                           content:content
                                           trigger:nil];

  notification_request_ = request;
  if (electron::debug_notifications) {
    LOG(INFO) << "Notification created (" << [identifier UTF8String] << ")";
  }

  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::SequencedTaskRunner::GetCurrentDefault();
  auto weak_self = GetWeakPtr();

  [[UNUserNotificationCenter currentNotificationCenter]
      addNotificationRequest:request
       withCompletionHandler:^(NSError* _Nullable error) {
         if (error) {
           if (electron::debug_notifications) {
             LOG(INFO) << "Error scheduling notification ("
                       << [identifier UTF8String] << ") "
                       << [error.localizedDescription UTF8String];
           }
         } else {
           task_runner->PostTask(
               FROM_HERE, base::BindOnce(
                              [](base::WeakPtr<Notification> weak_self) {
                                if (Notification* self = weak_self.get()) {
                                  CocoaNotification* un_self =
                                      static_cast<CocoaNotification*>(self);
                                  un_self->NotificationDisplayed();
                                }
                              },
                              weak_self));
           if (electron::debug_notifications) {
             LOG(INFO) << "Notification scheduled (" << [identifier UTF8String]
                       << ")";
           }
         }
       }];
}

void CocoaNotification::Dismiss() {
  if (notification_request_)
    [[UNUserNotificationCenter currentNotificationCenter]
        removeDeliveredNotificationsWithIdentifiers:@[
          notification_request_.identifier
        ]];

  NotificationDismissed();

  notification_request_ = nil;
}

void CocoaNotification::NotificationDisplayed() {
  if (delegate())
    delegate()->NotificationDisplayed();

  this->LogAction("displayed");
}

void CocoaNotification::NotificationReplied(const std::string& reply) {
  if (delegate())
    delegate()->NotificationReplied(reply);

  this->LogAction("replied to");
}

void CocoaNotification::NotificationActivated(int actionIndex) {
  if (delegate())
    delegate()->NotificationAction(actionIndex);

  this->LogAction("button clicked");
}

void CocoaNotification::NotificationDismissed() {
  if (delegate())
    delegate()->NotificationClosed();

  this->LogAction("dismissed");
}

void CocoaNotification::LogAction(const char* action) {
  if (electron::debug_notifications && notification_request_) {
    NSString* identifier = [notification_request_ valueForKey:@"identifier"];
    DCHECK(identifier);
    LOG(INFO) << "Notification " << action << " (" << [identifier UTF8String]
              << ")";
  }
}

}  // namespace electron
