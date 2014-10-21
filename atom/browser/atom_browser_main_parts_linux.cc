// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_main_parts.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "library_loaders/libgio.h"
#include "ui/gfx/switches.h"

namespace atom {

namespace {

const char* kInterfaceSchema = "org.gnome.desktop.interface";
const char* kScaleFactor = "scale-factor";

}  // namespace

void AtomBrowserMainParts::SetDPIFromGSettings() {
  LibGioLoader libgio_loader;

  // Try also without .0 at the end; on some systems this may be required.
  if (!libgio_loader.Load("libgio-2.0.so.0") &&
      !libgio_loader.Load("libgio-2.0.so")) {
    VLOG(1) << "Cannot load gio library. Will fall back to gconf.";
    return;
  }

  GSettings* client = libgio_loader.g_settings_new(kInterfaceSchema);
  if (!client) {
    VLOG(1) << "Cannot create gsettings client.";
    return;
  }

  gchar** keys = libgio_loader.g_settings_list_keys(client);
  if (!keys) {
    g_object_unref(client);
    return;
  }

  // Check if the "scale-factor" settings exsits.
  gchar** iter = keys;
  while (*iter) {
    if (strcmp(*iter, kScaleFactor) == 0)
      break;
    iter++;
  }

  if (*iter) {
    guint scale_factor = libgio_loader.g_settings_get_uint(
        client, kScaleFactor);
    if (scale_factor >= 1) {
      base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
          switches::kForceDeviceScaleFactor, base::UintToString(scale_factor));
    }
  }

  g_strfreev(keys);
  g_object_unref(client);
}

}  // namespace atom
