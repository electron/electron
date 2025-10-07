// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/plugins/plugin_utils.h"

#include <vector>

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
  return {};
}

base::flat_map<std::string, std::string>
PluginUtils::GetMimeTypeToExtensionIdMap(
    content::BrowserContext* browser_context) {
  base::flat_map<std::string, std::string> mime_type_to_extension_id_map;
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  const auto& allowed_extension_ids = MimeTypesHandler::GetMIMETypeAllowlist();
  if (allowed_extension_ids.empty())
    return {};

  const extensions::ExtensionSet& enabled_extensions =
      extensions::ExtensionRegistry::Get(browser_context)->enabled_extensions();

  // Go through the white-listed extensions and try to use them to intercept
  // the URL request.
  for (const std::string& id : allowed_extension_ids) {
    const extensions::Extension* extension = enabled_extensions.GetByID(id);
    if (!extension)  // extension might not be installed, so check for nullptr
      continue;

    if (const MimeTypesHandler* handler =
            MimeTypesHandler::GetHandler(extension)) {
      for (const std::string& mime_type : handler->mime_type_set()) {
        const auto [_, inserted] =
            mime_type_to_extension_id_map.insert_or_assign(mime_type, id);
        DCHECK(inserted);
      }
    }
  }
#endif

  return mime_type_to_extension_id_map;
}
