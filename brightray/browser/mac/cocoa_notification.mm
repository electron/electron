// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brightray/browser/mac/cocoa_notification.h"

#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "brightray/browser/notification_delegate.h"
#include "brightray/browser/notification_presenter.h"
#include "skia/ext/skia_utils_mac.h"

namespace brightray {

CocoaNotification::CocoaNotification(NotificationDelegate* delegate,
                                     NotificationPresenter* presenter)
    : Notification(delegate, presenter) {
}

CocoaNotification::~CocoaNotification() {
  if (notification_)
    [NSUserNotificationCenter.defaultUserNotificationCenter
        removeDeliveredNotification:notification_];
}

void CocoaNotification::Show(const base::string16& title,
                             const base::string16& body,
                             const std::string& tag,
                             const GURL& icon_url,
                             const SkBitmap& icon,
                             bool silent,
                             bool has_reply,
                             const base::string16& reply_placeholder) {
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

  if (has_reply) {
    [notification_ setResponsePlaceholder:base::SysUTF16ToNSString(reply_placeholder)];
    [notification_ setHasReplyButton:true];
  }

  [NSUserNotificationCenter.defaultUserNotificationCenter
      deliverNotification:notification_];
}

void CocoaNotification::Dismiss() {
  if (notification_)
    [NSUserNotificationCenter.defaultUserNotificationCenter
        removeDeliveredNotification:notification_];
  NotificationDismissed();
}

void CocoaNotification::NotificationDisplayed() {
  if (delegate())
    delegate()->NotificationDisplayed();
}

void CocoaNotification::NotificationReplied(const std::string& reply) {
  if (delegate())
    delegate()->NotificationReplied(reply);
}

}  // namespace brightray
