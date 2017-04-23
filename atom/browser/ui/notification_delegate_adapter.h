// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "browser/notification_delegate.h"

#include "atom/browser/api/atom_api_notification.h"

#ifndef ATOM_BROWSER_UI_NOTIFICATION_DELEGATE_ADAPTER_H_
#define ATOM_BROWSER_UI_NOTIFICATION_DELEGATE_ADAPTER_H_

namespace atom {

class AtomNotificationDelegateAdapter : public brightray::NotificationDelegate {
  public:
    atom::api::Notification* observer_;
    AtomNotificationDelegateAdapter(atom::api::Notification* target);

    void NotificationDisplayed();
    void NotificationClosed();
    void NotificationClick();
    void NotificationDestroyed();
    void NotificationFailed();
};

}

#endif // ATOM_BROWSER_UI_NOTIFICATION_DELEGATE_ADAPTER_H_