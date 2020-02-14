// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_PLUGINS_PLUGIN_UTILS_H_
#define SHELL_BROWSER_PLUGINS_PLUGIN_UTILS_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/macros.h"

namespace content {
class BrowserContext;
}

class PluginUtils {
 public:
  // If there's an extension that is allowed to handle |mime_type|, returns its
  // ID. Otherwise returns an empty string.
  static std::string GetExtensionIdForMimeType(
      content::BrowserContext* browser_context,
      const std::string& mime_type);

  // Returns a map populated with MIME types that are handled by an extension as
  // keys and the corresponding extensions Ids as values.
  static base::flat_map<std::string, std::string> GetMimeTypeToExtensionIdMap(
      content::BrowserContext* browser_context);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PluginUtils);
};

#endif  // SHELL_BROWSER_PLUGINS_PLUGIN_UTILS_H_
