// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_

#include <memory>
#include <string>

#include "extensions/browser/component_extension_resource_manager.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/extension_id.h"

static_assert(BUILDFLAG(ENABLE_EXTENSIONS_CORE));

namespace content {
class BrowserContext;
}

namespace extensions {

// Refs
// //chrome/browser/extensions/chrome_component_extension_resource_manager.h
class ElectronComponentExtensionResourceManager
    : public ComponentExtensionResourceManager {
 public:
  ElectronComponentExtensionResourceManager();

  ElectronComponentExtensionResourceManager(
      const ElectronComponentExtensionResourceManager&) = delete;
  ElectronComponentExtensionResourceManager& operator=(
      const ElectronComponentExtensionResourceManager&) = delete;

  ~ElectronComponentExtensionResourceManager() override;

  // Overridden from ComponentExtensionResourceManager:
  bool IsComponentExtensionResource(const base::FilePath& extension_path,
                                    const base::FilePath& resource_path,
                                    int* resource_id) const override;
  const ui::TemplateReplacements* GetTemplateReplacementsForExtension(
      const ExtensionId& extension_id,
      content::BrowserContext* context) const override;
  bool IsDynamicComponentExtensionResource(
      const ExtensionId& extension_id,
      const std::string& path,
      content::BrowserContext* context) const override;
  std::string GetDynamicResourceContent(
      const ExtensionId& extension_id,
      const std::string& path,
      content::BrowserContext* context) const override;

 private:
  class Data;

  void LazyInitData() const;

  // Logically const. Initialized on demand to keep browser start-up fast.
  mutable std::unique_ptr<const Data> data_;
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
