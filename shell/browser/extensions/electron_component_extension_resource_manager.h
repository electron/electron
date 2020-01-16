// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_

#include <stddef.h>

#include <map>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "extensions/browser/component_extension_resource_manager.h"

struct GritResourceMap;

namespace extensions {

class ElectronComponentExtensionResourceManager
    : public ComponentExtensionResourceManager {
 public:
  ElectronComponentExtensionResourceManager();
  ~ElectronComponentExtensionResourceManager() override;

  // Overridden from ComponentExtensionResourceManager:
  bool IsComponentExtensionResource(const base::FilePath& extension_path,
                                    const base::FilePath& resource_path,
                                    int* resource_id) const override;
  const ui::TemplateReplacements* GetTemplateReplacementsForExtension(
      const std::string& extension_id) const override;

 private:
  void AddComponentResourceEntries(const GritResourceMap* entries, size_t size);

  // A map from a resource path to the resource ID.  Used by
  // IsComponentExtensionResource.
  std::map<base::FilePath, int> path_to_resource_id_;

  // A map from an extension ID to its i18n template replacements.
  std::map<std::string, ui::TemplateReplacements>
      extension_template_replacements_;

  DISALLOW_COPY_AND_ASSIGN(ElectronComponentExtensionResourceManager);
};

}  // namespace extensions

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
