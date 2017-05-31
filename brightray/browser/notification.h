// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"

class GURL;
class SkBitmap;

namespace brightray {

class NotificationDelegate;
class NotificationPresenter;

class Notification {
 public:
  virtual ~Notification();

  // Shows the notification.
  virtual void Show(const base::string16& title,
                    const base::string16& msg,
                    const std::string& tag,
                    const GURL& icon_url,
                    const SkBitmap& icon,
                    bool silent,
                    bool has_reply,
                    const base::string16& reply_placeholder) = 0;
  // Closes the notification, this instance will be destroyed after the
  // notification gets closed.
  virtual void Dismiss() = 0;

  // Should be called by derived classes.
  void NotificationClicked();
  void NotificationDismissed();
  void NotificationFailed();

  // delete this.
  void Destroy();

  base::WeakPtr<Notification> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  void set_delegate(NotificationDelegate* delegate) { delegate_ = delegate; }
  NotificationDelegate* delegate() const { return delegate_; }
  NotificationPresenter* presenter() const { return presenter_; }

 protected:
  Notification(NotificationDelegate* delegate,
               NotificationPresenter* presenter);

 private:
  NotificationDelegate* delegate_;
  NotificationPresenter* presenter_;

  base::WeakPtrFactory<Notification> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Notification);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_NOTIFICATION_H_
