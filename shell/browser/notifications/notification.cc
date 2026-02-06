// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/notifications/notification.h"

#include "base/environment.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/notification_presenter.h"

namespace electron {

const bool debug_notifications =
    base::Environment::Create()->HasVar("ELECTRON_DEBUG_NOTIFICATIONS");

NotificationOptions::NotificationOptions() = default;
NotificationOptions::NotificationOptions(const NotificationOptions&) = default;
NotificationOptions& NotificationOptions::operator=(
    const NotificationOptions&) = default;
NotificationOptions::NotificationOptions(NotificationOptions&&) = default;
NotificationOptions& NotificationOptions::operator=(NotificationOptions&&) =
    default;
NotificationOptions::~NotificationOptions() = default;

NotificationAction::NotificationAction() = default;
NotificationAction::~NotificationAction() = default;
NotificationAction::NotificationAction(const NotificationAction&) = default;
NotificationAction& NotificationAction::operator=(const NotificationAction&) =
    default;
NotificationAction::NotificationAction(NotificationAction&&) noexcept = default;
NotificationAction& NotificationAction::operator=(
    NotificationAction&&) noexcept = default;

Notification::Notification(NotificationDelegate* delegate,
                           NotificationPresenter* presenter)
    : delegate_(delegate), presenter_(presenter) {}

Notification::~Notification() {
  if (delegate())
    delegate()->NotificationDestroyed();
}

void Notification::NotificationClicked() {
  if (delegate())
    delegate()->NotificationClick();
  Destroy();
}

void Notification::NotificationDismissed(bool should_destroy) {
  if (delegate())
    delegate()->NotificationClosed();

  set_is_dismissed(true);

  if (should_destroy)
    Destroy();
}

void Notification::NotificationFailed(const std::string& error) {
  if (delegate())
    delegate()->NotificationFailed(error);
  Destroy();
}

void Notification::Destroy() {
  if (presenter()) {
    presenter()->RemoveNotification(this);
  }
}

}  // namespace electron
