// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_notification.h"

#import <Foundation/Foundation.h>

#include "atom/browser/browser.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"

std::map<int, base::scoped_nsobject<NSUserNotification>> native_notifications_;

@interface AtomNotificationCenter : NSObject<NSUserNotificationCenterDelegate> {
}
@end

@implementation AtomNotificationCenter
- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center
     shouldPresentNotification:(NSUserNotification *)notification {
    return YES;
  }

- (void)userNotificationCenter:(NSUserNotificationCenter *)center 
       didActivateNotification:(NSUserNotification *)notification {
    int n_id = [[notification.userInfo objectForKey:@"id"] intValue];
    if (atom::api::Notification::HasID(n_id)) {
      auto atomNotification = atom::api::Notification::FromID(n_id);

      if (notification.activationType == NSUserNotificationActivationTypeReplied){
        atomNotification->OnReplied([notification.response.string UTF8String]);
      } else {
        atomNotification->OnClicked(); 
      }
    }
  }
@end

namespace atom {

namespace api {

AtomNotificationCenter* del = [[AtomNotificationCenter alloc] init];
bool set_del_ = false;

void Notification::Show() {
  base::scoped_nsobject<NSUserNotification> notification_ = native_notifications_[id_];
  [NSUserNotificationCenter.defaultUserNotificationCenter
      deliverNotification:notification_];
  OnShown();
}

void Notification::OnInitialProps() {
  if (!set_del_) {
    [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:del];
    set_del_ = true;
  }

  base::scoped_nsobject<NSUserNotification> notification_;
  notification_.reset([[NSUserNotification alloc] init]);

  native_notifications_[id_] = notification_;

  NotifyPropsUpdated();
}

void Notification::NotifyPropsUpdated() {
  base::scoped_nsobject<NSUserNotification> notification_ = native_notifications_[id_];

  [notification_ setTitle:base::SysUTF16ToNSString(title_)];
  [notification_ setInformativeText:base::SysUTF16ToNSString(body_)];

  NSDictionary * userInfo = [NSMutableDictionary dictionary];
  [userInfo setValue:[NSNumber numberWithInt:id_] forKey:@"id"];
  [notification_ setUserInfo:userInfo];

  if ([notification_ respondsToSelector:@selector(setContentImage:)] && has_icon_) {
    [notification_ setContentImage:icon_.AsNSImage()];
  }

  if (has_reply_) {
    [notification_ setResponsePlaceholder:base::SysUTF16ToNSString(reply_placeholder_)];
    [notification_ setHasReplyButton:true];
  }

  if (silent_) {
    [notification_ setSoundName:nil];
  } else {
    [notification_ setSoundName:NSUserNotificationDefaultSoundName];
  }
}

}  // namespace api

}  // namespace atom