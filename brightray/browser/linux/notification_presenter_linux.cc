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

const char *kRenderProcessIDKey = "RenderProcessID";
const char *kRenderViewIDKey = "RenderViewID";
const char *kNotificationIDKey = "NotificationID";

void log_and_clear_error(GError *error, const char *context) {
  if (error) {
    LOG(ERROR) << context << ": domain=" << error->domain << " code=" << error->code << " message=\"" << error->message << "\"";
    g_error_free(error);
  }
}

void closed_cb(NotifyNotification *notification, NotificationPresenterLinux *obj) {
  int render_process_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(notification), kRenderProcessIDKey));
  int render_view_id    = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(notification), kRenderViewIDKey));
  int notification_id   = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(notification), kNotificationIDKey));
  LOG(INFO) << "closed_cb: process=" << render_process_id
            << " view=" << render_view_id
            << " notification=" << notification_id
            << " reason=" << notify_notification_get_closed_reason(notification);
  obj->RemoveNotification(notification);
}

void action_cb(NotifyNotification *notification, const char *action, NotificationPresenterLinux *obj) {
  int render_process_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(notification), kRenderProcessIDKey));
  int render_view_id    = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(notification), kRenderViewIDKey));
  int notification_id   = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(notification), kNotificationIDKey));
  LOG(INFO) << "action_cb: process=" << render_process_id
            << " view=" << render_view_id
            << " notification=" << notification_id
            << " action=\"" << action << '"';
  auto host = content::RenderViewHost::FromID(render_process_id, render_view_id);
  if (host) host->DesktopNotificationPostClick(notification_id);
  obj->RemoveNotification(notification);
}

}

NotificationPresenter* NotificationPresenter::Create() {
  if (!notify_is_initted()) {
    notify_init(GetApplicationName().c_str());
  }
  return new NotificationPresenterLinux;
}

NotificationPresenterLinux::NotificationPresenterLinux() : notifications_(NULL) { }

NotificationPresenterLinux::~NotificationPresenterLinux() {
  if (notifications_) {
    for (GList *p = notifications_; p != NULL; p = p->next) {
      g_object_unref(G_OBJECT(p->data));
    }
    g_list_free(notifications_);
  }
}

void NotificationPresenterLinux::ShowNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    int render_process_id,
    int render_view_id) {
  LOG(INFO) << "ShowNotification: process=" << render_process_id
            << " view=" << render_view_id
            << " notification=" << params.notification_id
            << " title=\"" << params.title << '"'
            << " body=\"" << params.body << '"';

  std::string title = base::UTF16ToUTF8(params.title);
  std::string body = base::UTF16ToUTF8(params.body);
  NotifyNotification *notification = notify_notification_new(title.c_str(), body.c_str(), NULL);
  g_object_set_data(G_OBJECT(notification), kRenderProcessIDKey, GINT_TO_POINTER(render_process_id));
  g_object_set_data(G_OBJECT(notification), kRenderViewIDKey,    GINT_TO_POINTER(render_view_id));
  g_object_set_data(G_OBJECT(notification), kNotificationIDKey,  GINT_TO_POINTER(params.notification_id));
  g_signal_connect(notification, "closed", G_CALLBACK(closed_cb), this);
  notify_notification_add_action(notification, "default", "View", (NotifyActionCallback)action_cb, this, NULL);

  notifications_ = g_list_append(notifications_, notification);

  GError *error = NULL;
  notify_notification_show(notification, &error);
  log_and_clear_error(error, "notify_notification_show");

  auto host = content::RenderViewHost::FromID(render_process_id, render_view_id);
  if (!host)
    return;

  host->DesktopNotificationPostDisplay(params.notification_id);
}

void NotificationPresenterLinux::CancelNotification(
    int render_process_id,
    int render_view_id,
    int notification_id) {
  LOG(INFO) << "CancelNotification: process=" << render_process_id
            << " view=" << render_view_id
            << " notification=" << notification_id;

  NotifyNotification *notification = NULL;
  for (GList *p = notifications_; p != NULL; p = p->next) {
    if (render_process_id == GPOINTER_TO_INT(g_object_get_data(G_OBJECT(p->data), kRenderProcessIDKey))
     && render_view_id    == GPOINTER_TO_INT(g_object_get_data(G_OBJECT(p->data), kRenderViewIDKey))
     && notification_id   == GPOINTER_TO_INT(g_object_get_data(G_OBJECT(p->data), kNotificationIDKey))) {
      notification = (NotifyNotification*)p->data;
      notifications_ = g_list_delete_link(notifications_, p);
      break;
    }
  }
  if (!notification)
    return;

  GError *error = NULL;
  notify_notification_close(notification, &error);
  log_and_clear_error(error, "notify_notification_close");
  g_object_unref(notification);

  auto host = content::RenderViewHost::FromID(render_process_id, render_view_id);
  if (!host)
    return;

  host->DesktopNotificationPostClose(notification_id, false);
}

void NotificationPresenterLinux::RemoveNotification(NotifyNotification *former_notification) {
  notifications_ = g_list_remove(notifications_, former_notification);
  g_object_unref(former_notification);
}

}
