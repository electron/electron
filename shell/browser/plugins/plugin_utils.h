// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PLUGINS_PLUGIN_UTILS_H_
#define ELECTRON_SHELL_BROWSER_PLUGINS_PLUGIN_UTILS_H_

#include <string>

#include "base/containers/flat_map.h"

namespace content {
class BrowserContext;
}

class PluginUtils {
 public:
  // disable copy
  PluginUtils(const PluginUtils&) = delete;
  PluginUtils& operator=(const PluginUtils&) = delete;

  // If there's an extension that is allowed to handle |mime_type|, returns its
  // ID. Otherwise returns an empty string.
  static std::string GetExtensionIdForMimeType(
      content::BrowserContext* browser_context,
      const std::string& mime_type);

  // Returns a map populated with MIME types that are handled by an extension as
  // keys and the corresponding extensions Ids as values.
  static base::flat_map<std::string, std::string> GetMimeTypeToExtensionIdMap(
      content::BrowserContext* browser_context);
};

#endif  // ELECTRON_SHELL_BROWSER_PLUGINS_PLUGIN_UTILS_H_
