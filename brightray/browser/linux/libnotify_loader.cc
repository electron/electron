// This is generated file. Do not modify directly.
// Path to the code generator: tools/generate_library_loader/generate_library_loader.py .

#include "browser/linux/libnotify_loader.h"

#include <dlfcn.h>

LibNotifyLoader::LibNotifyLoader() : loaded_(false) {
}

LibNotifyLoader::~LibNotifyLoader() {
  CleanUp(loaded_);
}

bool LibNotifyLoader::Load(const std::string& library_name) {
  if (loaded_)
    return false;

  library_ = dlopen(library_name.c_str(), RTLD_LAZY);
  if (!library_)
    return false;

  notify_is_initted =
      reinterpret_cast<decltype(this->notify_is_initted)>(
          dlsym(library_, "notify_is_initted"));
  notify_is_initted = &::notify_is_initted;
  if (!notify_is_initted) {
    CleanUp(true);
    return false;
  }

  notify_init =
      reinterpret_cast<decltype(this->notify_init)>(
          dlsym(library_, "notify_init"));
  notify_init = &::notify_init;
  if (!notify_init) {
    CleanUp(true);
    return false;
  }

  notify_get_server_info =
      reinterpret_cast<decltype(this->notify_get_server_info)>(
          dlsym(library_, "notify_get_server_info"));
  notify_get_server_info = &::notify_get_server_info;
  if (!notify_get_server_info) {
    CleanUp(true);
    return false;
  }

  notify_get_server_caps =
      reinterpret_cast<decltype(this->notify_get_server_caps)>(
          dlsym(library_, "notify_get_server_caps"));
  notify_get_server_caps = &::notify_get_server_caps;
  if (!notify_get_server_caps) {
    CleanUp(true);
    return false;
  }

  notify_notification_new =
      reinterpret_cast<decltype(this->notify_notification_new)>(
          dlsym(library_, "notify_notification_new"));
  notify_notification_new = &::notify_notification_new;
  if (!notify_notification_new) {
    CleanUp(true);
    return false;
  }

  notify_notification_add_action =
      reinterpret_cast<decltype(this->notify_notification_add_action)>(
          dlsym(library_, "notify_notification_add_action"));
  notify_notification_add_action = &::notify_notification_add_action;
  if (!notify_notification_add_action) {
    CleanUp(true);
    return false;
  }

  notify_notification_set_image_from_pixbuf =
      reinterpret_cast<decltype(this->notify_notification_set_image_from_pixbuf)>(
          dlsym(library_, "notify_notification_set_image_from_pixbuf"));
  notify_notification_set_image_from_pixbuf = &::notify_notification_set_image_from_pixbuf;
  if (!notify_notification_set_image_from_pixbuf) {
    CleanUp(true);
    return false;
  }

  notify_notification_set_timeout =
      reinterpret_cast<decltype(this->notify_notification_set_timeout)>(
          dlsym(library_, "notify_notification_set_timeout"));
  notify_notification_set_timeout = &::notify_notification_set_timeout;
  if (!notify_notification_set_timeout) {
    CleanUp(true);
    return false;
  }

  notify_notification_set_hint_string =
      reinterpret_cast<decltype(this->notify_notification_set_hint_string)>(
          dlsym(library_, "notify_notification_set_hint_string"));
  notify_notification_set_hint_string = &::notify_notification_set_hint_string;
  if (!notify_notification_set_hint_string) {
    CleanUp(true);
    return false;
  }

  notify_notification_show =
      reinterpret_cast<decltype(this->notify_notification_show)>(
          dlsym(library_, "notify_notification_show"));
  notify_notification_show = &::notify_notification_show;
  if (!notify_notification_show) {
    CleanUp(true);
    return false;
  }

  notify_notification_close =
      reinterpret_cast<decltype(this->notify_notification_close)>(
          dlsym(library_, "notify_notification_close"));
  notify_notification_close = &::notify_notification_close;
  if (!notify_notification_close) {
    CleanUp(true);
    return false;
  }

  loaded_ = true;
  return true;
}

void LibNotifyLoader::CleanUp(bool unload) {
  if (unload) {
    dlclose(library_);
    library_ = NULL;
  }
  loaded_ = false;
  notify_is_initted = NULL;
  notify_init = NULL;
  notify_get_server_info = NULL;
  notify_get_server_caps = NULL;
  notify_notification_new = NULL;
  notify_notification_add_action = NULL;
  notify_notification_set_image_from_pixbuf = NULL;
  notify_notification_set_timeout = NULL;
  notify_notification_set_hint_string = NULL;
  notify_notification_show = NULL;
  notify_notification_close = NULL;
}
