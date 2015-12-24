// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "browser/mac/cocoa_notification.h"

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/mac/mac_util.h"
#include "base/stl_util.h"
#include "base/strings/sys_string_conversions.h"
#include "browser/mac/notification_delegate.h"
#include "skia/ext/skia_utils_mac.h"

namespace brightray {

// static
base::scoped_nsobject<NotificationDelegate>
CocoaNotification::notification_delegate_;

// static
std::set<CocoaNotification*> CocoaNotification::notifications_;

// static
CocoaNotification* CocoaNotification::FromNSNotification(
    NSUserNotification* ns_notification) {
  for (CocoaNotification* notification : notifications_) {
    if ([notification->notification_ isEqual:ns_notification])
      return notification;
  }
  return nullptr;
}

// static
void CocoaNotification::Cleanup() {
  NSUserNotificationCenter.defaultUserNotificationCenter.delegate = nil;
  notification_delegate_.reset();
  STLDeleteElements(&notifications_);
}

CocoaNotification::CocoaNotification(
    scoped_ptr<content::DesktopNotificationDelegate> delegate)
    : delegate_(delegate.Pass()),
      weak_factory_(this) {
  if (!notification_delegate_) {
    notification_delegate_.reset([[NotificationDelegate alloc] init]);
    NSUserNotificationCenter.defaultUserNotificationCenter.delegate =
        notification_delegate_;
    base::AtExitManager::RegisterTask(base::Bind(Cleanup));
  }

  notifications_.insert(this);
}

CocoaNotification::~CocoaNotification() {
  [NSUserNotificationCenter.defaultUserNotificationCenter
      removeDeliveredNotification:notification_];
  notifications_.erase(this);
}

void CocoaNotification::ShowNotification(const base::string16& title,
                                         const base::string16& body,
                                         const SkBitmap& icon) {
  notification_.reset([[NSUserNotification alloc] init]);
  [notification_ setTitle:base::SysUTF16ToNSString(title)];
  [notification_ setInformativeText:base::SysUTF16ToNSString(body)];

  if ([notification_ respondsToSelector:@selector(setContentImage:)] &&
      !icon.drawsNothing()) {
    NSImage* image = gfx::SkBitmapToNSImageWithColorSpace(
        icon, base::mac::GetGenericRGBColorSpace());
    [notification_ setContentImage:image];
  }

  [NSUserNotificationCenter.defaultUserNotificationCenter
      deliverNotification:notification_];
}

void CocoaNotification::DismissNotification() {
  delete this;
}

void CocoaNotification::NotifyDisplayed() {
  delegate_->NotificationDisplayed();
}

void CocoaNotification::NotifyClick() {
  delegate_->NotificationClick();
  delete this;
}

}  // namespace brightray
