// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#import "browser/notification_presenter_mac.h"

#import "base/stringprintf.h"
#import "base/strings/sys_string_conversions.h"
#import "content/public/browser/render_view_host.h"
#import "content/public/common/show_desktop_notification_params.h"

#import <Foundation/Foundation.h>

@interface BRYUserNotificationCenterDelegate : NSObject <NSUserNotificationCenterDelegate>
@end

namespace brightray {

namespace {

NSString * const kRenderProcessIDKey = @"RenderProcessID";
NSString * const kRenderViewIDKey = @"RenderViewID";
NSString * const kNotificationIDKey = @"NotificationID";

struct NotificationID {
  NotificationID(
      int render_process_id,
      int render_view_id,
      int notification_id)
      : render_process_id(render_process_id),
        render_view_id(render_view_id),
        notification_id(notification_id) {
  }

  NotificationID(NSUserNotification* notification)
      : render_process_id([[notification.userInfo objectForKey:kRenderProcessIDKey] intValue]),
        render_view_id([[notification.userInfo objectForKey:kRenderViewIDKey] intValue]),
        notification_id([[notification.userInfo objectForKey:kNotificationIDKey] intValue]) {
  }

  std::string GetID() {
    return base::StringPrintf("%d:%d:%d", render_process_id, render_view_id, notification_id);
  }

  NSDictionary* GetUserInfo() {
    return @{
      kRenderProcessIDKey: @(render_process_id),
      kRenderViewIDKey: @(render_view_id),
      kNotificationIDKey: @(notification_id),
    };
  }

  int render_process_id;
  int render_view_id;
  int notification_id;
};

scoped_nsobject<NSUserNotification> CreateUserNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    int render_process_id,
    int render_view_id) {
  auto notification = [[NSUserNotification alloc] init];
  notification.title = base::SysUTF16ToNSString(params.title);
  notification.informativeText = base::SysUTF16ToNSString(params.body);
  notification.userInfo = NotificationID(render_process_id, render_view_id, params.notification_id).GetUserInfo();

  return scoped_nsobject<NSUserNotification>(notification);
}

}

NotificationPresenter* NotificationPresenter::Create() {
  return new NotificationPresenterMac;
}

NotificationPresenterMac::NotificationPresenterMac()
    : delegate_([[BRYUserNotificationCenterDelegate alloc] init]) {
  NSUserNotificationCenter.defaultUserNotificationCenter.delegate = delegate_;
}

NotificationPresenterMac::~NotificationPresenterMac() {
}

void NotificationPresenterMac::ShowNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    int render_process_id,
    int render_view_id) {
  auto notification = CreateUserNotification(params, render_process_id, render_view_id);
  notification_map_.insert(std::make_pair(NotificationID(notification).GetID(), notification));
  [NSUserNotificationCenter.defaultUserNotificationCenter deliverNotification:notification];
}

void NotificationPresenterMac::CancelNotification(
    int render_process_id,
    int render_view_id,
    int notification_id) {
  auto found = notification_map_.find(NotificationID(render_process_id, render_view_id, notification_id).GetID());
  if (found == notification_map_.end())
    return;

  auto notification = found->second;

  notification_map_.erase(found);

  // Notifications in -deliveredNotifications aren't the same objects we passed to
  // -deliverNotification:, but they will respond YES to -isEqual:.
  auto center = NSUserNotificationCenter.defaultUserNotificationCenter;
  for (NSUserNotification* deliveredNotification in center.deliveredNotifications) {
    if (![notification isEqual:deliveredNotification])
      continue;
    [center removeDeliveredNotification:deliveredNotification];
  }

  NotificationID ID(notification);
  auto host = content::RenderViewHost::FromID(ID.render_process_id, ID.render_view_id);
  if (!host)
    return;

  host->DesktopNotificationPostClose(ID.notification_id, false);
}

}

@implementation BRYUserNotificationCenterDelegate

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didDeliverNotification:(NSUserNotification *)notification {
  brightray::NotificationID ID(notification);

  auto host = content::RenderViewHost::FromID(ID.render_process_id, ID.render_view_id);
  if (!host)
    return;

  host->DesktopNotificationPostDisplay(ID.notification_id);
}

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification {
  brightray::NotificationID ID(notification);

  auto host = content::RenderViewHost::FromID(ID.render_process_id, ID.render_view_id);
  if (!host)
    return;

  host->DesktopNotificationPostClick(ID.notification_id);
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification {
  // Display notifications even if the app is active.
  return YES;
}

@end
