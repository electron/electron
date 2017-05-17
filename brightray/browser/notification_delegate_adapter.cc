// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "browser/notification_delegate_adapter.h"

namespace brightray {

NotificationDelegateAdapter::NotificationDelegateAdapter(
    std::unique_ptr<content::DesktopNotificationDelegate> delegate)
    : delegate_(std::move(delegate)) {
}

NotificationDelegateAdapter::~NotificationDelegateAdapter() {
}

void NotificationDelegateAdapter::NotificationDestroyed() {
  delete this;
}

void NotificationDelegateAdapter::NotificationDisplayed() {
  delegate_->NotificationDisplayed();
}

void NotificationDelegateAdapter::NotificationClosed() {
  delegate_->NotificationClosed();
}

void NotificationDelegateAdapter::NotificationClick() {
  delegate_->NotificationClick();
}

}  // namespace brightray
