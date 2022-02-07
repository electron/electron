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
#include "shell/browser/notifications/mac/notification_center_delegate.h"
#include "shell/browser/notifications/mac/notification_presenter_mac.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/notification_presenter.h"
#include "skia/ext/skia_utils_mac.h"

namespace electron {

CocoaNotification::CocoaNotification(NotificationDelegate* delegate,
                                     NotificationPresenter* presenter)
    : Notification(delegate, presenter),
      is_persistent_(false),
      allow_destroy_(false) {}

CocoaNotification::~CocoaNotification() {
  // SAP-14881 - Persistent notification on macOS
  // exclusion auto deletion notifications to allow cold start from toast for
  // the App
  if (allow_destroy_)
    Dismiss();
}

void CocoaNotification::Show(const NotificationOptions& options) {
  notification_.reset([[[NSUserNotification alloc] init] retain]);

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

  [notification_ setHasActionButton:false];

  NSMutableArray* additionalActions =
      [[[NSMutableArray alloc] init] autorelease];

  NSString* replyKey_ = @"";
  for (const auto& action : options.actions) {
    // SAP-15908 - Refine notification-activation for cold start in macOS
    if (action.type == electron::NotificationAction::sTYPE_TEXT)
      replyKey_ = [[NSString alloc]
          initWithBytes:action.arg.data()
                 length:action.arg.length() * sizeof(action.arg[0])
               encoding:NSUTF16LittleEndianStringEncoding];
    // SAP-15259 - Add the reply field on a notification
    if (action.type != electron::NotificationAction::sTYPE_BUTTON)
      continue;
    // SAP-14881 - Persistent notification on macOS
    NSString* actionIdentifier = [[NSString alloc]
        initWithBytes:action.arg.data()
               length:action.arg.length() * sizeof(action.arg[0])
             encoding:NSUTF16LittleEndianStringEncoding];
    NSUserNotificationAction* notificationAction = [NSUserNotificationAction
        actionWithIdentifier:actionIdentifier
                       title:base::SysUTF16ToNSString(action.text)];
    [additionalActions addObject:notificationAction];
    additional_action_indices_.insert(
        std::make_pair(base::SysNSStringToUTF8(actionIdentifier),
                       additional_action_indices_.size()));
  }

  if ([additionalActions count] > 0) {
    [notification_ setHasActionButton:true];
    // SAP-14881 - Persistent notification on macOS
    [notification_ setActionButtonTitle:@"More"];
    [notification_ setValue:@YES forKey:@"_alwaysShowAlternateActionMenu"];
    [notification_ setAdditionalActions:additionalActions];
  }

  NSMutableDictionary* userInfo = [[NSMutableDictionary alloc] init];
  if (options.has_reply) {
    [notification_ setResponsePlaceholder:base::SysUTF16ToNSString(
                                              options.reply_placeholder)];
    [notification_ setHasReplyButton:true];

    // SAP-15908 - Refine notification-activation for cold start in macOS
    [userInfo setObject:replyKey_ forKey:@"_replyKey"];
  }

  // SAP-20223 [N] TECH [MACOS] To support renotify on macOS
  [userInfo setObject:(options.should_be_presented ? @"YES" : @"NO")
               forKey:@"_shouldBePresented"];

  if (!options.close_button_text.empty()) {
    [notification_ setOtherButtonTitle:base::SysUTF16ToNSString(
                                           options.close_button_text)];
  }

  if (!options.data.empty()) {
    // SAP-14775 - Support data property
    NSString* dataStr = [[NSString alloc]
        initWithBytes:options.data.c_str()
               length:options.data.length() * sizeof(options.data[0])
             encoding:NSUTF16LittleEndianStringEncoding];
    // https://developer.apple.com/documentation/foundation/nsusernotification/1415675-userinfo?language=objc
    // The userInfo content must be of reasonable serialized size (less than
    // 1KB) or an exception is thrown.
    [userInfo setObject:dataStr forKey:@"_data"];
  }
  [notification_ setUserInfo:userInfo];

  // SAP-14036 upgrade for persistent notifications support
  // SAP-17772: configuration toast show time
  // according to Notification.requireInteraction property
  is_persistent_ = options.is_persistent && options.require_interaction;

  if (is_persistent_) {
    deliverXPCNotification(notification_);
  } else {
    [NSUserNotificationCenter.defaultUserNotificationCenter
        deliverNotification:notification_];
  }
  NSLog(@"CocoaNotification::Show : %@", notification_.get());
}

void CocoaNotification::deliverXPCNotification(
    NSUserNotification* notification) {
  NotificationPresenterMac* presenter_ =
      (NotificationPresenterMac*)CocoaNotification::presenter();
  NotificationCenterDelegate* delegate_ = presenter_->delegate();
  [[[delegate_ connection] remoteObjectProxy] deliverNotification:notification];
  // Code bellow is diagnostics and will be removed after XPC will be fully
  // tested
  [[[delegate_ connection] remoteObjectProxy]
      upperCaseString:notification.title
            withReply:^(NSString* aString) {
              // We have received a response. Update our text field, but do it
              // on the main thread.
              NSLog(@"Result string was: %@", aString);
            }];
}

void CocoaNotification::removeXPCNotification(
    NSUserNotification* notification) {
  NotificationPresenterMac* presenter_ =
      (NotificationPresenterMac*)CocoaNotification::presenter();
  NotificationCenterDelegate* delegate_ = presenter_->delegate();
  [[[delegate_ connection] remoteObjectProxy]
      removeDeliveredNotification:notification];
  // Code bellow is diagnostics and will be removed after XPC will be fully
  // tested
  [[[delegate_ connection] remoteObjectProxy]
      upperCaseString:notification.title
            withReply:^(NSString* aString) {
              // We have received a response. Update our text field, but do it
              // on the main thread.
              NSLog(@"Result string was: %@", aString);
            }];
}

void CocoaNotification::Dismiss() {
  if (notification_) {
    if (is_persistent_) {
      removeXPCNotification(notification_);
    } else {
      [NSUserNotificationCenter.defaultUserNotificationCenter
          removeDeliveredNotification:notification_];
    }
  }

  NotificationDismissed();

  notification_.reset(nil);
}

void CocoaNotification::Destroy() {
  allow_destroy_ = true;
  Notification::Destroy();
}

void CocoaNotification::NotificationDisplayed() {
  if (delegate())
    delegate()->NotificationDisplayed();

  this->LogAction("displayed");
}

void CocoaNotification::NotificationClicked() {
  // SAP-14036 upgrade for persistent notifications support
  if (!is_persistent_)
    Notification::NotificationClicked();
  else if (delegate())
    delegate()->NotificationAction(-1);  // simple click

  this->LogAction("clicked");
}

void CocoaNotification::NotificationReplied(const std::string& reply) {
  if (delegate())
    delegate()->NotificationReplied(reply);

  this->LogAction("replied to");
}

void CocoaNotification::NotificationActivated() {
  if (delegate())
    delegate()->NotificationAction(0);

  this->LogAction("button clicked");
}

void CocoaNotification::NotificationActivated(
    NSUserNotificationAction* action) {
  if (delegate()) {
    int index = -1;
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
    delegate()->NotificationClosed(is_persistent_);

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
