// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/notifications/linux/libnotify_notification.h"

#include <string>

#include "base/containers/flat_set.h"
#include "base/files/file_enumerator.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/process/process_handle.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/ui/gtk_util.h"
#include "shell/common/application_info.h"
#include "shell/common/platform_util.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace electron {

namespace {

LibNotifyLoader& GetLibNotifyLoader() {
  static base::NoDestructor<LibNotifyLoader> loader;
  return *loader;
}

const base::flat_set<std::string>& GetServerCapabilities() {
  static base::NoDestructor<base::flat_set<std::string>> caps;
  if (caps->empty()) {
    auto* capabilities = GetLibNotifyLoader().notify_get_server_caps();
    for (auto* l = capabilities; l != nullptr; l = l->next)
      caps->insert(static_cast<const char*>(l->data));
    g_list_free_full(capabilities, g_free);
  }
  return *caps;
}

bool HasCapability(const std::string& capability) {
  return GetServerCapabilities().contains(capability);
}

bool NotifierSupportsActions() {
  if (getenv("ELECTRON_USE_UBUNTU_NOTIFIER"))
    return false;

  return HasCapability("actions");
}

void log_and_clear_error(GError* error, const char* context) {
  LOG(ERROR) << context << ": domain=" << error->domain
             << " code=" << error->code << " message=\"" << error->message
             << '"';
  g_error_free(error);
}

}  // namespace

// static
bool LibnotifyNotification::Initialize() {
  if (!GetLibNotifyLoader().Load("libnotify.so.4") &&  // most common one
      !GetLibNotifyLoader().Load("libnotify.so.5") &&
      !GetLibNotifyLoader().Load("libnotify.so.1") &&
      !GetLibNotifyLoader().Load("libnotify.so")) {
    LOG(WARNING) << "Unable to find libnotify; notifications disabled";
    return false;
  }
  if (!GetLibNotifyLoader().notify_is_initted() &&
      !GetLibNotifyLoader().notify_init(GetApplicationName().c_str())) {
    LOG(WARNING) << "Unable to initialize libnotify; notifications disabled";
    return false;
  }
  return true;
}

LibnotifyNotification::LibnotifyNotification(NotificationDelegate* delegate,
                                             NotificationPresenter* presenter)
    : Notification(delegate, presenter) {}

LibnotifyNotification::~LibnotifyNotification() {
  if (notification_) {
    g_signal_handlers_disconnect_by_data(notification_, this);
    g_object_unref(notification_);
  }
}

void LibnotifyNotification::Show(const NotificationOptions& options) {
  notification_ = GetLibNotifyLoader().notify_notification_new(
      base::UTF16ToUTF8(options.title).c_str(),
      base::UTF16ToUTF8(options.msg).c_str(), nullptr);

  signal_ = ScopedGSignal(
      notification_, "closed",
      base::BindRepeating(&LibnotifyNotification::OnNotificationClosed,
                          base::Unretained(this)));

  // NB: On Unity and on any other DE using Notify-OSD, adding a notification
  // action will cause the notification to display as a modal dialog box.
  if (NotifierSupportsActions()) {
    GetLibNotifyLoader().notify_notification_add_action(
        notification_, "default", "View", OnNotificationView, this, nullptr);
  }

  NotifyUrgency urgency = NOTIFY_URGENCY_NORMAL;
  if (options.urgency == u"critical") {
    urgency = NOTIFY_URGENCY_CRITICAL;
  } else if (options.urgency == u"low") {
    urgency = NOTIFY_URGENCY_LOW;
  }

  // Set the urgency level of the notification.
  GetLibNotifyLoader().notify_notification_set_urgency(notification_, urgency);

  if (!options.icon.drawsNothing()) {
    GdkPixbuf* pixbuf = gtk_util::GdkPixbufFromSkBitmap(options.icon);
    GetLibNotifyLoader().notify_notification_set_image_from_pixbuf(
        notification_, pixbuf);
    g_object_unref(pixbuf);
  }

  // Set the timeout duration for the notification
  bool neverTimeout = options.timeout_type == u"never";
  int timeout = (neverTimeout) ? NOTIFY_EXPIRES_NEVER : NOTIFY_EXPIRES_DEFAULT;
  GetLibNotifyLoader().notify_notification_set_timeout(notification_, timeout);

  if (!options.tag.empty()) {
    GQuark id = g_quark_from_string(options.tag.c_str());
    g_object_set(G_OBJECT(notification_), "id", id, nullptr);
  }

  // Always try to append notifications.
  // Unique tags can be used to prevent this.
  if (HasCapability("append")) {
    GetLibNotifyLoader().notify_notification_set_hint(
        notification_, "append", g_variant_new_string("true"));
  } else if (HasCapability("x-canonical-append")) {
    GetLibNotifyLoader().notify_notification_set_hint(
        notification_, "x-canonical-append", g_variant_new_string("true"));
  }

  // https://specifications.freedesktop.org/notification/latest-single/#id-1.9.7
  // This specifies the name of the desktop filename representing the
  // calling program. This should be the same as the prefix used for the
  // application's .desktop file. An example would be "rhythmbox" from
  // "rhythmbox.desktop". This can be used by the daemon to retrieve the
  // correct icon for the application, for logging purposes, etc.
  if (const std::optional<std::string> name = platform_util::GetXdgAppId()) {
    GetLibNotifyLoader().notify_notification_set_hint(
        notification_, "desktop-entry", g_variant_new_string(name->c_str()));
  }

  GetLibNotifyLoader().notify_notification_set_hint(
      notification_, "sender-pid",
      g_variant_new_int64(base::GetCurrentProcId()));

  GError* error = nullptr;
  GetLibNotifyLoader().notify_notification_show(notification_, &error);
  if (error) {
    log_and_clear_error(error, "notify_notification_show");
    NotificationFailed();
    return;
  }

  if (delegate())
    delegate()->NotificationDisplayed();
}

void LibnotifyNotification::Dismiss() {
  if (!notification_) {
    return;
  }

  GError* error = nullptr;
  on_dismissing_ = true;
  GetLibNotifyLoader().notify_notification_close(notification_, &error);
  if (error) {
    log_and_clear_error(error, "notify_notification_close");
  }
  on_dismissing_ = false;
}

void LibnotifyNotification::OnNotificationClosed(
    NotifyNotification* notification) {
  NotificationDismissed(!on_dismissing_);
}

void LibnotifyNotification::OnNotificationView(NotifyNotification* notification,
                                               char* action,
                                               gpointer user_data) {
  LibnotifyNotification* that = static_cast<LibnotifyNotification*>(user_data);
  DCHECK(that);
  that->NotificationClicked();
}

}  // namespace electron
