// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BROWSER_NOTIFICATION_DELEGATE_ADAPTER_H_
#define BROWSER_NOTIFICATION_DELEGATE_ADAPTER_H_

#include <memory>

#include "base/macros.h"
#include "browser/notification_delegate.h"

namespace brightray {

// Adapt the content::DesktopNotificationDelegate to NotificationDelegate.
class NotificationDelegateAdapter : public NotificationDelegate {
 public:
  explicit NotificationDelegateAdapter(
      std::unique_ptr<content::DesktopNotificationDelegate> delegate);
  ~NotificationDelegateAdapter() override;

  // NotificationDelegate:
  void NotificationDestroyed() override;

  // content::DesktopNotificationDelegate:
  void NotificationDisplayed() override;
  void NotificationClosed() override;
  void NotificationClick() override;

 private:
  std::unique_ptr<content::DesktopNotificationDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(NotificationDelegateAdapter);
};

}  // namespace brightray

#endif  // BROWSER_NOTIFICATION_DELEGATE_ADAPTER_H_
