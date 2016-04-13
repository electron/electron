// This is generated file. Do not modify directly.
// Path to the code generator: tools/generate_library_loader/generate_library_loader.py .

#ifndef BRIGHTRAY_BROWSER_LINUX_LIBNOTIFY_LOADER_H_
#define BRIGHTRAY_BROWSER_LINUX_LIBNOTIFY_LOADER_H_

#include <string>

#include <libnotify/notify.h>

class LibNotifyLoader {
 public:
  LibNotifyLoader();
  ~LibNotifyLoader();

  bool Load(const std::string& library_name)
      __attribute__((warn_unused_result));

  bool loaded() const { return loaded_; }

  decltype(&::notify_is_initted) notify_is_initted;
  decltype(&::notify_init) notify_init;
  decltype(&::notify_get_server_info) notify_get_server_info;
  decltype(&::notify_notification_new) notify_notification_new;
  decltype(&::notify_notification_add_action) notify_notification_add_action;
  decltype(&::notify_notification_set_image_from_pixbuf) notify_notification_set_image_from_pixbuf;
  decltype(&::notify_notification_set_timeout) notify_notification_set_timeout;
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

#endif  // BRIGHTRAY_BROWSER_LINUX_LIBNOTIFY_LOADER_H_
