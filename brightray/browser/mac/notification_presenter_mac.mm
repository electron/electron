// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#import "browser/mac/notification_presenter_mac.h"

#include "base/bind.h"
#include "browser/mac/cocoa_notification.h"
#include "content/public/common/platform_notification_data.h"

namespace brightray {

namespace {

void RemoveNotification(base::WeakPtr<Notification> notification) {
  if (notification)
    notification->DismissNotification();
}

}  // namespace

NotificationPresenter* NotificationPresenter::Create() {
  return new NotificationPresenterMac;
}

NotificationPresenterMac::NotificationPresenterMac() {
}

NotificationPresenterMac::~NotificationPresenterMac() {
}

void NotificationPresenterMac::ShowNotification(
    const content::PlatformNotificationData& data,
    const SkBitmap& icon,
    scoped_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback) {
  // This class manages itself.
  auto notification = new CocoaNotification(delegate.Pass());
  notification->ShowNotification(data.title, data.body, icon);

  if (cancel_callback) {
    *cancel_callback = base::Bind(
        &RemoveNotification, notification->GetWeakPtr());
  }
}

}  // namespace brightray
