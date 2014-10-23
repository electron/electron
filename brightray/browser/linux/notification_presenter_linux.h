// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Patrick Reynolds <piki@github.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_LINUX_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_LINUX_H_

#include <libnotify/notify.h>

#include <map>

#include "base/compiler_specific.h"
#include "browser/notification_presenter.h"
#include "ui/base/glib/glib_signal.h"

namespace brightray {

class NotificationPresenterLinux : public NotificationPresenter {
 public:
  NotificationPresenterLinux();
  ~NotificationPresenterLinux();

  void RemoveNotification(NotifyNotification *notification);

 private:
  // NotificationPresenter:
  virtual void ShowNotification(
      const content::ShowDesktopNotificationHostMsgParams&,
      scoped_ptr<content::DesktopNotificationDelegate> delegate,
      base::Closure* cancel_callback) override;

  void CancelNotification(NotifyNotification* notification);
  void DeleteNotification(NotifyNotification* notification);

  CHROMEG_CALLBACK_0(NotificationPresenterLinux, void, OnNotificationClosed, NotifyNotification*);
  CHROMEG_CALLBACK_1(NotificationPresenterLinux, void, OnNotificationView, NotifyNotification*,
                     char*);

  // A list of all open NotifyNotification objects.
  // We do lookups here both by NotifyNotification object (when the user
  // clicks a notification) and by the <process,view,notification> ID
  // tuple (when the browser asks to dismiss a notification).  So it's not
  // a map.
  // Entries in this list count as refs, so removal from this list should
  // always go with g_object_unref().
  GList* notifications_;
};

}  // namespace brightray

#endif
