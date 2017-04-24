// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_notification.h"

#include <libnotify/notify.h>

#include "atom/browser/browser.h"
#include "browser/linux/libnotify_loader.h"
#include "browser/linux/libnotify_notification.h"
#include "chrome/browser/ui/libgtkui/skia_utils_gtk.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace atom {

namespace api {

bool initialized_ = false;
bool available_ = false;

LibNotifyLoader libnotify_loader_;

bool HasCapability(const std::string& capability) {
  bool result = false;
  GList* capabilities = libnotify_loader_.notify_get_server_caps();

  if (g_list_find_custom(capabilities, capability.c_str(),
                         (GCompareFunc)g_strcmp0) != NULL)
    result = true;

  g_list_free_full(capabilities, g_free);

  return result;
}

bool NotifierSupportsActions() {
  if (getenv("ELECTRON_USE_UBUNTU_NOTIFIER"))
    return false;

  static bool notify_has_result = false;
  static bool notify_result = false;

  if (notify_has_result)
    return notify_result;

  notify_result = HasCapability("actions");
  return notify_result;
}

void log_and_clear_error(GError* error, const char* context) {
  LOG(ERROR) << context
             << ": domain=" << error->domain
             << " code=" << error->code
             << " message=\"" << error->message << '"';
  g_error_free(error);
}

void Notification::Show() {
  if (!available_) return;

  NotifyNotification* notification_ = libnotify_loader_.notify_notification_new(
      base::UTF16ToUTF8(title_).c_str(),
      base::UTF16ToUTF8(body_).c_str(),
      nullptr);

  GQuark id = g_quark_from_string(std::to_string(id_).c_str());
  g_object_set(G_OBJECT(notification_), "id", id, NULL);

  // NB: On Unity and on any other DE using Notify-OSD, adding a notification
  // action will cause the notification to display as a modal dialog box.
  // Can't make this work, need linux help :D
  // if (NotifierSupportsActions()) {
  //   libnotify_loader_.notify_notification_add_action(
  //       notification_, "default", "View", OnClickedCallback, this,
  //       nullptr);
  // }

  if (has_icon_) {
    SkBitmap image = *(icon_.ToSkBitmap());
    GdkPixbuf* pixbuf = libgtkui::GdkPixbufFromSkBitmap(image);
    libnotify_loader_.notify_notification_set_image_from_pixbuf(
        notification_, pixbuf);
    libnotify_loader_.notify_notification_set_timeout(
        notification_, NOTIFY_EXPIRES_DEFAULT);
    g_object_unref(pixbuf);
  }

  // Always try to append notifications.
  // Unique tags can be used to prevent this.
  if (HasCapability("append")) {
    libnotify_loader_.notify_notification_set_hint_string(
        notification_, "append", "true");
  } else if (HasCapability("x-canonical-append")) {
    libnotify_loader_.notify_notification_set_hint_string(
        notification_, "x-canonical-append", "true");
  }

  GError* error = nullptr;
  libnotify_loader_.notify_notification_show(notification_, &error);
  if (error) {
    log_and_clear_error(error, "notify_notification_show");
    return;
  }

  OnShown();
}

void Notification::OnInitialProps() {
  if (!initialized_) {
    initialized_ = true;
    if (!libnotify_loader_.Load("libnotify.so.4") &&  // most common one
        !libnotify_loader_.Load("libnotify.so.5") &&
        !libnotify_loader_.Load("libnotify.so.1") &&
        !libnotify_loader_.Load("libnotify.so")) {
      return;
    }
    Browser* browser = Browser::Get();
    if (!libnotify_loader_.notify_is_initted() &&
        !libnotify_loader_.notify_init(browser->GetName().c_str())) {
      return;
    }
    available_ = true;
  }
}

void Notification::NotifyPropsUpdated() {}
}  // namespace api
}  // namespace atom
