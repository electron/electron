// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/application_info.h"

#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>

#include <memory>
#include <string>

#include "atom/common/atom_version.h"
#include "atom/common/platform_util.h"
#include "base/environment.h"
#include "base/logging.h"
#include "chrome/browser/ui/libgtkui/gtk_util.h"

namespace {

GDesktopAppInfo* get_desktop_app_info() {
  GDesktopAppInfo* ret = nullptr;

  std::string desktop_id;
  if (platform_util::GetDesktopName(&desktop_id))
    ret = g_desktop_app_info_new(desktop_id.c_str());

  return ret;
}

}  // namespace

namespace atom {

std::string GetApplicationName() {
  // attempt #1: the string set in app.setName()
  std::string ret = GetOverriddenApplicationName();

  // attempt #2: the 'Name' entry from .desktop file's [Desktop] section
  if (ret.empty()) {
    GDesktopAppInfo* info = get_desktop_app_info();
    if (info != nullptr) {
      char* str = g_desktop_app_info_get_string(info, "Name");
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
    LOG(WARNING) << "No version found. Was app.setVersion() called?";
    ret = "0.0";
  }

  return ret;
}

}  // namespace atom
