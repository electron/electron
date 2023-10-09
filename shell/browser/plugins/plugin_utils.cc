// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/plugins/plugin_utils.h"

#include <vector>

#include "base/containers/contains.h"
#include "base/values.h"
#include "content/public/common/webplugininfo.h"
#include "electron/buildflags/buildflags.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/mime_types_handler.h"
#endif

// static
std::string PluginUtils::GetExtensionIdForMimeType(
    content::BrowserContext* browser_context,
    const std::string& mime_type) {
  auto map = GetMimeTypeToExtensionIdMap(browser_context);
  auto it = map.find(mime_type);
  if (it != map.end())
    return it->second;
  return std::string();
}

base::flat_map<std::string, std::string>
PluginUtils::GetMimeTypeToExtensionIdMap(
    content::BrowserContext* browser_context) {
  base::flat_map<std::string, std::string> mime_type_to_extension_id_map;
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  std::vector<std::string> allowed_extension_ids =
      MimeTypesHandler::GetMIMETypeAllowlist();
  // Go through the white-listed extensions and try to use them to intercept
  // the URL request.
  for (const std::string& extension_id : allowed_extension_ids) {
    const extensions::Extension* extension =
        extensions::ExtensionRegistry::Get(browser_context)
            ->enabled_extensions()
            .GetByID(extension_id);
    // The white-listed extension may not be installed, so we have to nullptr
    // check |extension|.
    if (!extension) {
      continue;
    }

    if (MimeTypesHandler* handler = MimeTypesHandler::GetHandler(extension)) {
      for (const auto& supported_mime_type : handler->mime_type_set()) {
        DCHECK(!base::Contains(mime_type_to_extension_id_map,
                               supported_mime_type));
        mime_type_to_extension_id_map[supported_mime_type] = extension_id;
      }
    }
  }
#endif
  return mime_type_to_extension_id_map;
}
