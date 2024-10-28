// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Patrick Reynolds <piki@github.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/notifications/linux/notification_presenter_linux.h"

#include "shell/browser/notifications/linux/libnotify_notification.h"

namespace electron {

// static
std::unique_ptr<NotificationPresenter> NotificationPresenter::Create() {
  if (LibnotifyNotification::Initialize())
    return std::make_unique<NotificationPresenterLinux>();
  return {};
}

NotificationPresenterLinux::NotificationPresenterLinux() = default;

NotificationPresenterLinux::~NotificationPresenterLinux() = default;

Notification* NotificationPresenterLinux::CreateNotificationObject(
    NotificationDelegate* delegate) {
  return new LibnotifyNotification(delegate, this);
}

}  // namespace electron
