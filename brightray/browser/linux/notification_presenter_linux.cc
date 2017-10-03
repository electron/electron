// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Patrick Reynolds <piki@github.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brightray/browser/linux/notification_presenter_linux.h"

#include "brightray/browser/linux/libnotify_notification.h"

namespace brightray {

// static
NotificationPresenter* NotificationPresenter::Create() {
  if (!LibnotifyNotification::Initialize())
    return nullptr;
  return new NotificationPresenterLinux;
}

NotificationPresenterLinux::NotificationPresenterLinux() {
}

NotificationPresenterLinux::~NotificationPresenterLinux() {
}

Notification* NotificationPresenterLinux::CreateNotificationObject(
    NotificationDelegate* delegate) {
  return new LibnotifyNotification(delegate, this);
}

}  // namespace brightray
