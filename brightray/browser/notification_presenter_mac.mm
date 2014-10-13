// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#import "browser/notification_presenter_mac.h"

#include "base/bind.h"
#include "base/stl_util.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/show_desktop_notification_params.h"

#import <Foundation/Foundation.h>

@interface BRYUserNotificationCenterDelegate : NSObject<NSUserNotificationCenterDelegate> {
 @private
  brightray::NotificationPresenterMac* presenter_;
}
- (instancetype)initWithNotificationPresenter:(brightray::NotificationPresenterMac*)presenter;
@end

namespace brightray {

namespace {

}  // namespace

NotificationPresenter* NotificationPresenter::Create() {
  return new NotificationPresenterMac;
}

NotificationPresenterMac::NotificationPresenterMac()
    : delegate_([[BRYUserNotificationCenterDelegate alloc] initWithNotificationPresenter:this]) {
  NSUserNotificationCenter.defaultUserNotificationCenter.delegate = delegate_;
}

NotificationPresenterMac::~NotificationPresenterMac() {
  NSUserNotificationCenter.defaultUserNotificationCenter.delegate = nil;
}

void NotificationPresenterMac::ShowNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    scoped_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback) {
  auto notification = [[NSUserNotification alloc] init];
  notification.title = base::SysUTF16ToNSString(params.title);
  notification.informativeText = base::SysUTF16ToNSString(params.body);

  notifications_map_[delegate.get()].reset(notification);
  [NSUserNotificationCenter.defaultUserNotificationCenter deliverNotification:notification];

  if (cancel_callback)
    *cancel_callback = base::Bind(
        &NotificationPresenterMac::CancelNotification,
        base::Unretained(this),
        delegate.release());
}

content::DesktopNotificationDelegate* NotificationPresenterMac::GetDelegateFromNotification(
    NSUserNotification* notification) {
  for (NotificationsMap::const_iterator it = notifications_map_.begin();
       it != notifications_map_.end(); ++it)
    if ([it->second isEqual:notification])
      return it->first;
  return NULL;
}

void NotificationPresenterMac::RemoveNotification(content::DesktopNotificationDelegate* delegate) {
  if (ContainsKey(notifications_map_, delegate)) {
    delete delegate;
    notifications_map_.erase(delegate);
  }
}

void NotificationPresenterMac::CancelNotification(content::DesktopNotificationDelegate* delegate) {
  if (!ContainsKey(notifications_map_, delegate))
    return;

  // Notifications in -deliveredNotifications aren't the same objects we passed to
  // -deliverNotification:, but they will respond YES to -isEqual:.
  auto notification = notifications_map_[delegate];
  auto center = NSUserNotificationCenter.defaultUserNotificationCenter;
  for (NSUserNotification* deliveredNotification in center.deliveredNotifications)
    if ([notification isEqual:deliveredNotification]) {
      [center removeDeliveredNotification:deliveredNotification];
      delegate->NotificationClosed(false);
      break;
    }

  RemoveNotification(delegate);
}

}  // namespace brightray

@implementation BRYUserNotificationCenterDelegate

- (instancetype)initWithNotificationPresenter:(brightray::NotificationPresenterMac*)presenter {
  self = [super init];
  if (!self)
    return nil;

  presenter_ = presenter;
  return self;
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
        didDeliverNotification:(NSUserNotification*)notification {
  auto delegate = presenter_->GetDelegateFromNotification(notification);
  if (delegate)
    delegate->NotificationDisplayed();
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
       didActivateNotification:(NSUserNotification *)notification {
  auto delegate = presenter_->GetDelegateFromNotification(notification);
  if (delegate) {
    delegate->NotificationClick();
    presenter_->RemoveNotification(delegate);
  }
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center
     shouldPresentNotification:(NSUserNotification*)notification {
  // Display notifications even if the app is active.
  return YES;
}

@end
