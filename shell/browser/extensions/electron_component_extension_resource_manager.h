// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
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
      const content::BrowserContext* context) const override;
  bool IsDynamicComponentExtensionResource(
      const ExtensionId& extension_id,
      const std::string& path,
      const content::BrowserContext* context) const override;
  std::string GetDynamicResourceContent(
      const ExtensionId& extension_id,
      const std::string& path,
      const content::BrowserContext* context) const override;

  [[nodiscard]] base::ScopedClosureRunner RegisterTemplateDataProvider(
      const ExtensionId& extension_id,
      const content::BrowserContext* context,
      TemplateDataProvider provider) const override;

 private:
  class Data;

  using ExtensionIdAndContext =
      std::pair<ExtensionId, const content::BrowserContext*>;
  void OnTemplateDataProviderRemoved(const ExtensionIdAndContext& key) const;

  void LazyInitData() const;

  // Logically const. Initialized on demand to keep browser start-up fast.
  mutable std::unique_ptr<const Data> data_;

  mutable std::map<ExtensionIdAndContext, TemplateDataProvider>
      template_data_providers_;
  mutable std::map<ExtensionIdAndContext, ui::TemplateReplacements>
      template_replacements_;

  mutable base::WeakPtrFactory<const ElectronComponentExtensionResourceManager>
      weak_factory_{this};
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
