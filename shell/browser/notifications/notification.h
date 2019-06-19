// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NOTIFICATIONS_NOTIFICATION_H_
#define ATOM_BROWSER_NOTIFICATIONS_NOTIFICATION_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "url/gurl.h"

namespace atom {

class NotificationDelegate;
class NotificationPresenter;

struct NotificationAction {
  base::string16 type;
  base::string16 text;
};

struct NotificationOptions {
  base::string16 title;
  base::string16 subtitle;
  base::string16 msg;
  std::string tag;
  bool silent;
  GURL icon_url;
  SkBitmap icon;
  bool has_reply;
  base::string16 reply_placeholder;
  base::string16 sound;
  std::vector<NotificationAction> actions;
  base::string16 close_button_text;

  NotificationOptions();
  ~NotificationOptions();
};

class Notification {
 public:
  virtual ~Notification();

  // Shows the notification.
  virtual void Show(const NotificationOptions& options) = 0;
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
  void set_notification_id(const std::string& id) { notification_id_ = id; }

  NotificationDelegate* delegate() const { return delegate_; }
  NotificationPresenter* presenter() const { return presenter_; }
  const std::string& notification_id() const { return notification_id_; }

 protected:
  Notification(NotificationDelegate* delegate,
               NotificationPresenter* presenter);

 private:
  NotificationDelegate* delegate_;
  NotificationPresenter* presenter_;
  std::string notification_id_;

  base::WeakPtrFactory<Notification> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Notification);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NOTIFICATIONS_NOTIFICATION_H_
