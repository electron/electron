// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/linux/notification_presenter_linux.h"

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/show_desktop_notification_params.h"
#include "common/application_info.h"

#include <libnotify/notify.h>

namespace brightray {

namespace {

struct NotificationID {
  NotificationID(
      int render_process_id,
      int render_view_id,
      int notification_id)
      : render_process_id(render_process_id),
        render_view_id(render_view_id),
        notification_id(notification_id) {
  }

  std::string GetID() {
    return base::StringPrintf("%d:%d:%d", render_process_id, render_view_id, notification_id);
  }

  int render_process_id;
  int render_view_id;
  int notification_id;
};

void log_and_clear_error(GError *error, const char *context) {
  if (error) {
    LOG(ERROR) << context << ": domain=" << error->domain << " code=" << error->code << " message=\"" << error->message << "\"";
    g_error_free(error);
  }
}

void closed_cb(NotifyNotification *notification, NotificationID *ID) {
  auto host = content::RenderViewHost::FromID(ID->render_process_id, ID->render_view_id);
  if (host) host->DesktopNotificationPostClick(ID->notification_id);
}

NotifyNotification *CreateUserNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    int render_process_id,
    int render_view_id) {
  std::string title = base::UTF16ToUTF8(params.title);
  std::string body = base::UTF16ToUTF8(params.body);
  NotifyNotification *notification = notify_notification_new(title.c_str(), body.c_str(), NULL);

  return notification;
}

}

NotificationPresenter* NotificationPresenter::Create() {
  if (!notify_is_initted()) {
    notify_init(GetApplicationName().c_str());
  }
  return new NotificationPresenterLinux;
}

void NotificationPresenterLinux::ShowNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    int render_process_id,
    int render_view_id) {
  DLOG(INFO) << "ShowNotification: process=" << render_process_id
             << " view=" << render_view_id
             << " notification=" << params.notification_id
             << " title=\"" << params.title << '"'
             << " body=\"" << params.body << '"';
  NotifyNotification *notification = CreateUserNotification(params, render_process_id, render_view_id);
  NotificationID ID(render_process_id, render_view_id, params.notification_id);
  std::pair<NotificationMap::iterator,bool> p = notification_map_.insert(std::make_pair(ID.GetID(), notification));

	g_signal_connect(notification, "closed", G_CALLBACK(closed_cb), new NotificationID(ID)); // FIXME: closure to free it

  GError *error = NULL;
  notify_notification_show(notification, &error);
  log_and_clear_error(error, "notify_notification_show");

  auto host = content::RenderViewHost::FromID(ID.render_process_id, ID.render_view_id);
  if (!host)
    return;

  host->DesktopNotificationPostDisplay(ID.notification_id);
}

void NotificationPresenterLinux::CancelNotification(
    int render_process_id,
    int render_view_id,
    int notification_id) {
  DLOG(INFO) << "CancelNotification: process=" << render_process_id
             << " view=" << render_view_id
             << " notification=" << notification_id;

  auto found = notification_map_.find(NotificationID(render_process_id, render_view_id, notification_id).GetID());
  if (found == notification_map_.end())
    return;

  auto notification = found->second;

  notification_map_.erase(found);

  GError *error = NULL;
  notify_notification_close(notification, &error);
  log_and_clear_error(error, "notify_notification_close");

  NotificationID ID(render_process_id, render_view_id, notification_id);
  auto host = content::RenderViewHost::FromID(render_process_id, render_view_id);
  if (!host)
    return;

  host->DesktopNotificationPostClose(ID.notification_id, false);
}

}
