// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_

#include <stddef.h>

#include <map>
#include <string>

#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "extensions/browser/component_extension_resource_manager.h"
#include "ui/base/webui/resource_path.h"

struct GritResourceMap;

namespace extensions {

class ElectronComponentExtensionResourceManager
    : public ComponentExtensionResourceManager {
 public:
  ElectronComponentExtensionResourceManager();
  ~ElectronComponentExtensionResourceManager() override;

  // disable copy
  ElectronComponentExtensionResourceManager(
      const ElectronComponentExtensionResourceManager&) = delete;
  ElectronComponentExtensionResourceManager& operator=(
      const ElectronComponentExtensionResourceManager&) = delete;

  // Overridden from ComponentExtensionResourceManager:
  bool IsComponentExtensionResource(const base::FilePath& extension_path,
                                    const base::FilePath& resource_path,
                                    int* resource_id) const override;
  const ui::TemplateReplacements* GetTemplateReplacementsForExtension(
      const std::string& extension_id) const override;

 private:
  void AddComponentResourceEntries(
      base::span<const webui::ResourcePath> entries);

  // A map from a resource path to the resource ID.  Used by
  // IsComponentExtensionResource.
  std::map<base::FilePath, int> path_to_resource_id_;

  // A map from an extension ID to its i18n template replacements.
  std::map<std::string, ui::TemplateReplacements>
      extension_template_replacements_;
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
