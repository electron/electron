// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/notifications/mac/cocoa_notification.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/notification_presenter.h"
#include "skia/ext/skia_utils_mac.h"

namespace electron {

CocoaNotification::CocoaNotification(NotificationDelegate* delegate,
                                     NotificationPresenter* presenter)
    : Notification(delegate, presenter) {}

CocoaNotification::~CocoaNotification() {
  if (notification_)
    [NSUserNotificationCenter.defaultUserNotificationCenter
        removeDeliveredNotification:notification_];
}

void CocoaNotification::Show(const NotificationOptions& options) {
  notification_ = [[NSUserNotification alloc] init];

  NSString* identifier =
      [NSString stringWithFormat:@"%@:notification:%@",
                                 [[NSBundle mainBundle] bundleIdentifier],
                                 [[NSUUID UUID] UUIDString]];

  [notification_ setTitle:base::SysUTF16ToNSString(options.title)];
  [notification_ setSubtitle:base::SysUTF16ToNSString(options.subtitle)];
  [notification_ setInformativeText:base::SysUTF16ToNSString(options.msg)];
  [notification_ setIdentifier:identifier];

  if (getenv("ELECTRON_DEBUG_NOTIFICATIONS")) {
    LOG(INFO) << "Notification created (" << [identifier UTF8String] << ")";
  }

  if (!options.icon.drawsNothing()) {
    NSImage* image = skia::SkBitmapToNSImageWithColorSpace(
        options.icon, base::mac::GetGenericRGBColorSpace());
    [notification_ setContentImage:image];
  }

  if (options.silent) {
    [notification_ setSoundName:nil];
  } else if (options.sound.empty()) {
    [notification_ setSoundName:NSUserNotificationDefaultSoundName];
  } else {
    [notification_ setSoundName:base::SysUTF16ToNSString(options.sound)];
  }

  if (options.has_reply) {
    [notification_ setHasReplyButton:true];
    [notification_ setResponsePlaceholder:base::SysUTF16ToNSString(
                                              options.reply_placeholder)];
  }

  // We need to explicitly set this to false if there are no
  // actions, otherwise a Show button will appear by default.
  if (options.actions.size() == 0)
    [notification_ setHasActionButton:false];

  int i = 0;
  action_index_ = UINT_MAX;
  NSMutableArray* additionalActions = [[NSMutableArray alloc] init];
  for (const auto& action : options.actions) {
    if (action.type == u"button") {
      // If the notification has both a reply and actions,
      // the reply takes precedence and the actions all
      // become additional actions.
      if (!options.has_reply && action_index_ == UINT_MAX) {
        // First button observed is the displayed action
        [notification_
            setActionButtonTitle:base::SysUTF16ToNSString(action.text)];
        action_index_ = i;
      } else {
        // All of the rest are appended to the list of additional actions
        NSString* actionIdentifier =
            [NSString stringWithFormat:@"%@Action%d", identifier, i];
        NSUserNotificationAction* notificationAction = [NSUserNotificationAction
            actionWithIdentifier:actionIdentifier
                           title:base::SysUTF16ToNSString(action.text)];
        [additionalActions addObject:notificationAction];
        additional_action_indices_.insert(
            std::make_pair(base::SysNSStringToUTF8(actionIdentifier), i));
      }
    }
    i++;
  }

  if ([additionalActions count] > 0) {
    [notification_ setAdditionalActions:additionalActions];
  }

  if (!options.close_button_text.empty()) {
    [notification_ setOtherButtonTitle:base::SysUTF16ToNSString(
                                           options.close_button_text)];
  }

  [NSUserNotificationCenter.defaultUserNotificationCenter
      deliverNotification:notification_];
}

void CocoaNotification::Dismiss() {
  if (notification_)
    [NSUserNotificationCenter.defaultUserNotificationCenter
        removeDeliveredNotification:notification_];

  NotificationDismissed();

  notification_ = nil;
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

void CocoaNotification::NotificationActivated() {
  if (delegate())
    delegate()->NotificationAction(action_index_);

  this->LogAction("button clicked");
}

void CocoaNotification::NotificationActivated(
    NSUserNotificationAction* action) {
  if (delegate()) {
    unsigned index = action_index_;
    std::string identifier = base::SysNSStringToUTF8(action.identifier);
    for (const auto& it : additional_action_indices_) {
      if (it.first == identifier) {
        index = it.second;
        break;
      }
    }

    delegate()->NotificationAction(index);
  }

  this->LogAction("button clicked");
}

void CocoaNotification::NotificationDismissed() {
  if (delegate())
    delegate()->NotificationClosed();

  this->LogAction("dismissed");
}

void CocoaNotification::LogAction(const char* action) {
  if (getenv("ELECTRON_DEBUG_NOTIFICATIONS") && notification_) {
    NSString* identifier = [notification_ valueForKey:@"identifier"];
    DCHECK(identifier);
    LOG(INFO) << "Notification " << action << " (" << [identifier UTF8String]
              << ")";
  }
}

}  // namespace electron
