// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Patrick Reynolds <piki@github.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include <libnotify/notify.h>

#include "browser/linux/notification_presenter_linux.h"

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/show_desktop_notification_params.h"
#include "common/application_info.h"

namespace brightray {

namespace {

const char *kRenderProcessIDKey = "RenderProcessID";
const char *kRenderViewIDKey = "RenderViewID";
const char *kNotificationIDKey = "NotificationID";

void log_and_clear_error(GError *error, const char *context) {
  if (!error) return;

  LOG(ERROR) << context
             << ": domain=" << error->domain
             << " code=" << error->code
             << " message=\"" << error->message << '"';
  g_error_free(error);
}

int GetObjectInt(NotifyNotification *noti, const char *key) {
  return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(noti), key));
}

void SetObjectInt(NotifyNotification *noti, const char *key, int value) {
  g_object_set_data(G_OBJECT(noti), key, GINT_TO_POINTER(value));
}


void NotificationClosedCallback(NotifyNotification *noti, NotificationPresenterLinux *obj) {
  int render_process_id = GetObjectInt(noti, kRenderProcessIDKey);
  int render_view_id    = GetObjectInt(noti, kRenderViewIDKey);
  int notification_id   = GetObjectInt(noti, kNotificationIDKey);

  auto host = content::RenderViewHost::FromID(render_process_id, render_view_id);
  if (host) host->DesktopNotificationPostClose(notification_id, false);
  obj->RemoveNotification(noti);
}

void NotificationViewCallback(NotifyNotification *noti, const char *action,
    NotificationPresenterLinux *obj) {
  int render_process_id = GetObjectInt(noti, kRenderProcessIDKey);
  int render_view_id    = GetObjectInt(noti, kRenderViewIDKey);
  int notification_id   = GetObjectInt(noti, kNotificationIDKey);

  auto host = content::RenderViewHost::FromID(render_process_id, render_view_id);
  if (host) host->DesktopNotificationPostClick(notification_id);
  obj->RemoveNotification(noti);
}

}  // namespace

NotificationPresenter* NotificationPresenter::Create() {
  if (!notify_is_initted()) {
    notify_init(GetApplicationName().c_str());
  }
  return new NotificationPresenterLinux;
}

NotificationPresenterLinux::NotificationPresenterLinux() : notifications_(nullptr) { }

NotificationPresenterLinux::~NotificationPresenterLinux() {
  // unref any outstanding notifications, and then free the list.
  if (!notifications_) return;

  for (GList *p = notifications_; p != nullptr; p = p->next) {
    g_object_unref(G_OBJECT(p->data));
  }
  g_list_free(notifications_);
}

void NotificationPresenterLinux::ShowNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    int render_process_id,
    int render_view_id) {
  std::string title = base::UTF16ToUTF8(params.title);
  std::string body = base::UTF16ToUTF8(params.body);
  NotifyNotification *noti = notify_notification_new(title.c_str(), body.c_str(), nullptr);
  SetObjectInt(noti, kRenderProcessIDKey, render_process_id);
  SetObjectInt(noti, kRenderViewIDKey,    render_view_id);
  SetObjectInt(noti, kNotificationIDKey,  params.notification_id);
  g_signal_connect(noti, "closed",
    G_CALLBACK(NotificationClosedCallback), this);
  notify_notification_add_action(noti, "default", "View",
    (NotifyActionCallback)NotificationViewCallback, this, nullptr);

  notifications_ = g_list_append(notifications_, noti);

  GError *error = nullptr;
  notify_notification_show(noti, &error);
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
  NotifyNotification *noti = nullptr;
  for (GList *p = notifications_; p != nullptr; p = p->next) {
    int rpid = GetObjectInt(noti, kRenderProcessIDKey);
    int rvid = GetObjectInt(noti, kRenderViewIDKey);
    int nid  = GetObjectInt(noti, kNotificationIDKey);
    if (render_process_id == rpid && render_view_id == rvid && notification_id == nid) {
      noti = reinterpret_cast<NotifyNotification*>(p->data);
      notifications_ = g_list_delete_link(notifications_, p);
      break;
    }
  }
  if (!noti)
    return;

  GError *error = nullptr;
  notify_notification_close(noti, &error);
  log_and_clear_error(error, "notify_notification_close");
  g_object_unref(noti);

  auto host = content::RenderViewHost::FromID(render_process_id, render_view_id);
  if (!host)
    return;

  host->DesktopNotificationPostClose(notification_id, false);
}

void NotificationPresenterLinux::RemoveNotification(NotifyNotification *noti) {
  notifications_ = g_list_remove(notifications_, noti);
  g_object_unref(noti);
}

}  // namespace brightray
