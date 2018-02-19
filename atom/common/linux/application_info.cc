// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "base/environment.h"
#if 0
#include "base/files/file_enumerator.h"
 #include "base/strings/string_util.h"
 #include "base/strings/utf_string_conversions.h"
 #include "brightray/browser/notification_delegate.h"
 #include "brightray/common/application_info.h"
#endif
#include "chrome/browser/ui/libgtkui/gtk_util.h"
// #include "chrome/browser/ui/libgtkui/skia_utils_gtk.h"
// #include "third_party/skia/include/core/SkBitmap.h"


#include "atom/common/atom_version.h"
#include "brightray/common/application_info.h"

namespace {

GDesktopAppInfo* get_desktop_app_info() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string desktop_id = libgtkui::GetDesktopName(env.get());
  return desktop_id.empty()
    ? nullptr
    : g_desktop_app_info_new (desktop_id.c_str());
}

}  // namespace

namespace brightray {

std::string GetApplicationName() {

  // attempt #1: the string set in app.setName()
  std::string ret = GetOverriddenApplicationName();

  // attempt #2: the 'Name' entry from .desktop file's [Desktop] section
  if (ret.empty()) {
    GDesktopAppInfo * info = get_desktop_app_info();
    if (info != nullptr) {
      char * str = g_desktop_app_info_get_string(info, "Name");
      g_clear_object(&info);
      if (str != nullptr)
        ret = str;
      g_clear_pointer(&str, g_free);
    }
  }

  // attempt #3: Electron's name
  if (ret.empty()) {
    ret = ATOM_PRODUCT_NAME;
  }

  return ret;
}

std::string GetApplicationVersion() {

  std::string ret;

  // ensure ATOM_PRODUCT_NAME and ATOM_PRODUCT_STRING match up
  if (GetApplicationName() == ATOM_PRODUCT_NAME)
    ret = ATOM_VERSION_STRING;

  // try to use the string set in app.setVersion()
  if (ret.empty())
    ret = GetOverriddenApplicationVersion();

  // no known version number; return some safe fallback
  if (ret.empty()) {
    g_warning("%s No version found. Was app.setVersion() called?", G_STRLOC);
    ret = "0.0";
  }

  return ret;
}

}  // namespace brightray
