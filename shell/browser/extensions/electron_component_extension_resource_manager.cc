// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_component_extension_resource_manager.h"

#include <map>
#include <string>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/component_extension_resources_map.h"
#include "chrome/grit/theme_resources.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_config_map.h"
#include "extensions/browser/extension_config_map_factory.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_id.h"
#include "pdf/buildflags.h"
#include "ui/base/resource/resource_bundle.h"

#if BUILDFLAG(ENABLE_PDF)
#include <utility>

#include "chrome/browser/pdf/pdf_extension_util.h"
#endif  // BUILDFLAG(ENABLE_PDF)

static_assert(BUILDFLAG(ENABLE_EXTENSIONS_CORE));

namespace extensions {
namespace {

ExtensionConfigProvider* GetConfigProvider(const ExtensionId& extension_id,
                                           content::BrowserContext* context) {
  if (!context) {
    return nullptr;
  }
  auto* config_map = ExtensionConfigMapFactory::GetForBrowserContext(context);
  return config_map ? config_map->GetConfigProvider(extension_id) : nullptr;
}

}  // namespace

class ElectronComponentExtensionResourceManager::Data {
 public:
  using TemplateReplacementMap =
      std::map<ExtensionId, ui::TemplateReplacements>;

  Data();
  Data(const Data&) = delete;
  Data& operator=(const Data&) = delete;
  ~Data() = default;

  const std::map<base::FilePath, int>& path_to_resource_id() const {
    return path_to_resource_id_;
  }

  const TemplateReplacementMap& template_replacements() const {
    return template_replacements_;
  }

 private:
  void AddComponentResourceEntries(
      base::span<const webui::ResourcePath> entries);

  // A map from a resource path to the resource ID. Used by
  // ElectronComponentExtensionResourceManager::IsComponentExtensionResource().
  std::map<base::FilePath, int> path_to_resource_id_;

  // A map from an extension ID to its i18n template replacements.
  TemplateReplacementMap template_replacements_;
};

ElectronComponentExtensionResourceManager::Data::Data() {
  static const webui::ResourcePath kExtraComponentExtensionResources[] = {
#if !BUILDFLAG(IS_CHROMEOS)
      {"web_store/webstore_icon_128.png", IDR_WEBSTORE_ICON},
      {"web_store/webstore_icon_16.png", IDR_WEBSTORE_ICON_16},
#endif
  };

  AddComponentResourceEntries(kComponentExtensionResources);
  AddComponentResourceEntries(kExtraComponentExtensionResources);

#if BUILDFLAG(ENABLE_PDF)
  AddComponentResourceEntries(pdf_extension_util::GetResources(
      pdf_extension_util::PdfViewerContext::kPdfViewer));

  // ResourceBundle is not always initialized in unit tests.
  if (ui::ResourceBundle::HasSharedInstance()) {
    base::DictValue dict = pdf_extension_util::GetStrings(
        pdf_extension_util::PdfViewerContext::kPdfViewer);

    ui::TemplateReplacements pdf_viewer_replacements;
    ui::TemplateReplacementsFromDictionaryValue(dict, &pdf_viewer_replacements);
    template_replacements_[extension_misc::kPdfExtensionId] =
        std::move(pdf_viewer_replacements);
  }
#endif
}

void ElectronComponentExtensionResourceManager::Data::
    AddComponentResourceEntries(base::span<const webui::ResourcePath> entries) {
  for (const auto& entry : entries) {
    base::FilePath resource_path = base::FilePath().AppendASCII(entry.path);
    resource_path = resource_path.NormalizePathSeparators();

    DCHECK(!path_to_resource_id_.contains(resource_path));
    path_to_resource_id_[resource_path] = entry.id;
  }
}

ElectronComponentExtensionResourceManager::
    ElectronComponentExtensionResourceManager() = default;

ElectronComponentExtensionResourceManager::
    ~ElectronComponentExtensionResourceManager() = default;

bool ElectronComponentExtensionResourceManager::IsComponentExtensionResource(
    const base::FilePath& extension_path,
    const base::FilePath& resource_path,
    int* resource_id) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::FilePath directory_path = extension_path;
  base::FilePath resources_dir;
  base::FilePath relative_path;
  if (!base::PathService::Get(chrome::DIR_RESOURCES, &resources_dir) ||
      !resources_dir.AppendRelativePath(directory_path, &relative_path)) {
    return false;
  }
  relative_path = relative_path.Append(resource_path);
  relative_path = relative_path.NormalizePathSeparators();

  LazyInitData();
  auto entry = data_->path_to_resource_id().find(relative_path);
  if (entry == data_->path_to_resource_id().end()) {
    return false;
  }

  *resource_id = entry->second;
  return true;
}

const ui::TemplateReplacements*
ElectronComponentExtensionResourceManager::GetTemplateReplacementsForExtension(
    const ExtensionId& extension_id,
    content::BrowserContext* context) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  LazyInitData();

  if (auto* provider = GetConfigProvider(extension_id, context)) {
    CHECK(context);
    return provider->GetTemplateReplacements(*context);
  }

  auto it = data_->template_replacements().find(extension_id);
  return it != data_->template_replacements().end() ? &it->second : nullptr;
}

bool ElectronComponentExtensionResourceManager::
    IsDynamicComponentExtensionResource(
        const ExtensionId& extension_id,
        const std::string& path,
        content::BrowserContext* context) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return path == kDynamicStringsJsPath &&
         GetConfigProvider(extension_id, context) != nullptr;
}

std::string
ElectronComponentExtensionResourceManager::GetDynamicResourceContent(
    const ExtensionId& extension_id,
    const std::string& path,
    content::BrowserContext* context) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK_EQ(path, kDynamicStringsJsPath);
  CHECK(context);

  auto* provider = GetConfigProvider(extension_id, context);
  CHECK(provider);

  base::DictValue dict = provider->GetLoadTimeData(*context);
  return base::StringPrintf(kDynamicStringsModuleTemplate,
                            base::WriteJson(dict).value_or("{}").c_str());
}

void ElectronComponentExtensionResourceManager::LazyInitData() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!data_)
    data_ = std::make_unique<Data>();
}

}  // namespace extensions
