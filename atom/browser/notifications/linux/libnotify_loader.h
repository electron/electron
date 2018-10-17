// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NOTIFICATIONS_LINUX_LIBNOTIFY_LOADER_H_
#define ATOM_BROWSER_NOTIFICATIONS_LINUX_LIBNOTIFY_LOADER_H_

// FIXME Generate during build using
// //tools/generate_library_loader/generate_library_loader.gni

#include <libnotify/notify.h>
#include <string>

class LibNotifyLoader {
 public:
  LibNotifyLoader();
  ~LibNotifyLoader();

  bool Load(const std::string& library_name)
      __attribute__((warn_unused_result));

  bool loaded() const { return loaded_; }

  decltype(&::notify_is_initted) notify_is_initted;
  decltype(&::notify_init) notify_init;
  decltype(&::notify_get_server_caps) notify_get_server_caps;
  decltype(&::notify_get_server_info) notify_get_server_info;
  decltype(&::notify_notification_new) notify_notification_new;
  decltype(&::notify_notification_add_action) notify_notification_add_action;
  decltype(&::notify_notification_set_image_from_pixbuf)
      notify_notification_set_image_from_pixbuf;
  decltype(&::notify_notification_set_timeout) notify_notification_set_timeout;
  decltype(&::notify_notification_set_hint_string)
      notify_notification_set_hint_string;
  decltype(&::notify_notification_show) notify_notification_show;
  decltype(&::notify_notification_close) notify_notification_close;

 private:
  void CleanUp(bool unload);

  void* library_;
  bool loaded_;

  // Disallow copy constructor and assignment operator.
  LibNotifyLoader(const LibNotifyLoader&);
  void operator=(const LibNotifyLoader&);
};

#endif  // ATOM_BROWSER_NOTIFICATIONS_LINUX_LIBNOTIFY_LOADER_H_
