// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/notifications/notification.h"

#include "atom/browser/notifications/notification_delegate.h"
#include "atom/browser/notifications/notification_presenter.h"

namespace atom {

NotificationOptions::NotificationOptions() = default;
NotificationOptions::~NotificationOptions() = default;

Notification::Notification(NotificationDelegate* delegate,
                           NotificationPresenter* presenter)
    : delegate_(delegate), presenter_(presenter), weak_factory_(this) {}

Notification::~Notification() {
  if (delegate())
    delegate()->NotificationDestroyed();
}

void Notification::NotificationClicked() {
  if (delegate())
    delegate()->NotificationClick();
  Destroy();
}

void Notification::NotificationDismissed() {
  if (delegate())
    delegate()->NotificationClosed();
  Destroy();
}

void Notification::NotificationFailed() {
  if (delegate())
    delegate()->NotificationFailed();
  Destroy();
}

void Notification::Destroy() {
  presenter()->RemoveNotification(this);
}

}  // namespace atom
