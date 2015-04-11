// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_main_parts.h"

#include <gio/gio.h>

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "ui/gfx/switches.h"

namespace atom {

namespace {

const char* kInterfaceSchema = "org.gnome.desktop.interface";
const char* kScaleFactor = "scaling-factor";

bool SchemaExists(const char* schema_name) {
  const gchar* const* schemas = g_settings_list_schemas();
  while (*schemas) {
    if (strcmp(schema_name, static_cast<const char*>(*schemas)) == 0)
      return true;
    schemas++;
  }
  return false;
}

bool KeyExists(GSettings* client, const char* key) {
  gchar** keys = g_settings_list_keys(client);
  if (!keys)
    return false;

  gchar** iter = keys;
  while (*iter) {
    if (strcmp(*iter, key) == 0)
      break;
    iter++;
  }

  bool exists = *iter != NULL;
  g_strfreev(keys);
  return exists;
}

void GetDPIFromGSettings(guint* scale_factor) {
  GSettings* client = nullptr;
  if (!SchemaExists(kInterfaceSchema) ||
      !(client = g_settings_new(kInterfaceSchema))) {
    VLOG(1) << "Cannot create gsettings client.";
    return;
  }

  if (KeyExists(client, kScaleFactor))
    *scale_factor = g_settings_get_uint(client, kScaleFactor);

  g_object_unref(client);
}

}  // namespace

void AtomBrowserMainParts::SetDPIFromGSettings() {
  guint scale_factor = 1;
  GetDPIFromGSettings(&scale_factor);
  if (scale_factor == 0)
    scale_factor = 1;

  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kForceDeviceScaleFactor, base::UintToString(scale_factor));
}

}  // namespace atom
