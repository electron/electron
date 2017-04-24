// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/notification_delegate_adapter.h"

#include "atom/browser/api/atom_api_notification.h"
#include "browser/notification_delegate.h"

namespace atom {

AtomNotificationDelegateAdapter::AtomNotificationDelegateAdapter(
    atom::api::Notification* target) {
  observer_ = target;
}
void AtomNotificationDelegateAdapter::NotificationDisplayed() {
  observer_->OnShown();
}
void AtomNotificationDelegateAdapter::NotificationClosed() {}
void AtomNotificationDelegateAdapter::NotificationClick() {
  observer_->OnClicked();
}
void AtomNotificationDelegateAdapter::NotificationDestroyed() {}
void AtomNotificationDelegateAdapter::NotificationFailed() {}

}  // namespace atom
