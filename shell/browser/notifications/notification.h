// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "url/gurl.h"

namespace electron {

class NotificationDelegate;
class NotificationPresenter;

struct NotificationAction {
  // feat: Add the reply field on a notification
  std::u16string type;  // type of the action (button or text input).
  std::u16string text;  // title of the button.
  std::u16string
      arg;    // argument that the author can use to distinguish actions.
  GURL icon;  // URL of the icon for the button. May be empty if
              // no icon was specified.
  // Optional text to use as placeholder for text inputs. May be empty if it was
  // not specified. Has meaning only for type == text input
  std::u16string placeholder;

  NotificationAction();
  NotificationAction(
      const NotificationAction& copy);  // requires for std::vector storing
  ~NotificationAction();
  static const std::u16string sTYPE_BUTTON;
  static const std::u16string sTYPE_TEXT;
};

struct NotificationOptions {
  std::u16string title;
  std::u16string subtitle;
  std::u16string msg;
  std::string tag;
  bool silent;
  GURL icon_url;
  SkBitmap icon;
  GURL image_url;
  SkBitmap image;
  bool has_reply;
  std::u16string timeout_type;
  std::u16string reply_placeholder;
  std::u16string sound;
  std::u16string urgency;  // Linux
  std::u16string data;     // feat: Support Notification.data property
  std::vector<NotificationAction> actions;
  std::u16string close_button_text;
  std::u16string toast_xml;
  bool is_persistent;
  bool require_interaction;
  bool should_be_presented;

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
  void NotificationFailed(const std::string& error = "");
  // feat: Notification was replied to
  void NotificationReplied(const std::string& reply);
  void NotificationAction(int index);

  // delete this.
  virtual void Destroy();

  base::WeakPtr<Notification> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  void set_delegate(NotificationDelegate* delegate) { delegate_ = delegate; }
  void set_notification_id(const std::string& id) { notification_id_ = id; }

  NotificationDelegate* delegate() const { return delegate_; }
  NotificationPresenter* presenter() const { return presenter_; }
  const std::string& notification_id() const { return notification_id_; }

  // disable copy
  Notification(const Notification&) = delete;
  Notification& operator=(const Notification&) = delete;

 protected:
  Notification(NotificationDelegate* delegate,
               NotificationPresenter* presenter);

 private:
  raw_ptr<NotificationDelegate> delegate_;
  raw_ptr<NotificationPresenter> presenter_;
  std::string notification_id_;

  base::WeakPtrFactory<Notification> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_H_
