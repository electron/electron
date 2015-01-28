// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Patrick Reynolds <piki@github.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/linux/notification_presenter_linux.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/show_desktop_notification_params.h"
#include "common/application_info.h"

namespace brightray {

namespace {

void log_and_clear_error(GError* error, const char* context) {
  LOG(ERROR) << context
             << ": domain=" << error->domain
             << " code=" << error->code
             << " message=\"" << error->message << '"';
  g_error_free(error);
}

content::DesktopNotificationDelegate* GetDelegateFromNotification(
    NotifyNotification* notification) {
  return static_cast<content::DesktopNotificationDelegate*>(
      g_object_get_data(G_OBJECT(notification), "delegate"));
}

}  // namespace

// static
NotificationPresenter* NotificationPresenter::Create() {
  if (!notify_is_initted()) {
    notify_init(GetApplicationName().c_str());
  }
  return new NotificationPresenterLinux;
}

NotificationPresenterLinux::NotificationPresenterLinux()
    : notifications_(nullptr) {
}

NotificationPresenterLinux::~NotificationPresenterLinux() {
  // unref any outstanding notifications, and then free the list.
  if (notifications_)
    g_list_free_full(notifications_, g_object_unref);
}

void NotificationPresenterLinux::ShowNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    scoped_ptr<content::DesktopNotificationDelegate> delegate_ptr,
    base::Closure* cancel_callback) {
  std::string title = base::UTF16ToUTF8(params.title);
  std::string body = base::UTF16ToUTF8(params.body);
  NotifyNotification* notification = notify_notification_new(title.c_str(), body.c_str(), nullptr);

  content::DesktopNotificationDelegate* delegate = delegate_ptr.release();

  g_object_set_data_full(G_OBJECT(notification), "delegate", delegate, operator delete);
  g_signal_connect(notification, "closed", G_CALLBACK(OnNotificationClosedThunk), this);
  notify_notification_add_action(notification, "default", "View", OnNotificationViewThunk, this,
                                 nullptr);

  GError* error = nullptr;
  notify_notification_show(notification, &error);
  if (error) {
    log_and_clear_error(error, "notify_notification_show");
    g_object_unref(notification);
    return;
  }

  notifications_ = g_list_append(notifications_, notification);
  delegate->NotificationDisplayed();

  if (cancel_callback)
    *cancel_callback = base::Bind(
        &NotificationPresenterLinux::CancelNotification,
        base::Unretained(this),
        notification);
}

void NotificationPresenterLinux::CancelNotification(NotifyNotification* notification) {
  GError* error = nullptr;
  notify_notification_close(notification, &error);
  if (error)
    log_and_clear_error(error, "notify_notification_close");

  GetDelegateFromNotification(notification)->NotificationClosed(false);
  DeleteNotification(notification);
}

void NotificationPresenterLinux::DeleteNotification(NotifyNotification* notification) {
  notifications_ = g_list_remove(notifications_, notification);
  g_object_unref(notification);
}

void NotificationPresenterLinux::OnNotificationClosed(NotifyNotification* notification) {
  GetDelegateFromNotification(notification)->NotificationClosed(false);
  DeleteNotification(notification);
}

void NotificationPresenterLinux::OnNotificationView(
    NotifyNotification* notification, char* action) {
  GetDelegateFromNotification(notification)->NotificationClick();
  DeleteNotification(notification);
}

}  // namespace brightray
