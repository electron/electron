// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_component_extension_resource_manager.h"

#include <string>
#include <utility>

#include "base/path_service.h"
#include "base/values.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/grit/component_extension_resources_map.h"
#include "electron/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "chrome/browser/pdf/pdf_extension_util.h"  // nogncheck
#include "chrome/grit/pdf_resources_map.h"
#include "extensions/common/constants.h"
#endif

namespace extensions {

ElectronComponentExtensionResourceManager::
    ElectronComponentExtensionResourceManager() {
  AddComponentResourceEntries(kComponentExtensionResources);
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  AddComponentResourceEntries(kPdfResources);

  // Register strings for the PDF viewer, so that $i18n{} replacements work.
  base::DictValue dict = pdf_extension_util::GetStrings(
      pdf_extension_util::PdfViewerContext::kPdfViewer);

  ui::TemplateReplacements pdf_viewer_replacements;
  ui::TemplateReplacementsFromDictionaryValue(dict, &pdf_viewer_replacements);
  template_replacements_[extension_misc::kPdfExtensionId] =
      std::move(pdf_viewer_replacements);
#endif
}

ElectronComponentExtensionResourceManager::
    ~ElectronComponentExtensionResourceManager() = default;

bool ElectronComponentExtensionResourceManager::IsComponentExtensionResource(
    const base::FilePath& extension_path,
    const base::FilePath& resource_path,
    int* resource_id) const {
  base::FilePath directory_path = extension_path;
  base::FilePath resources_dir;
  base::FilePath relative_path;
  if (!base::PathService::Get(chrome::DIR_RESOURCES, &resources_dir) ||
      !resources_dir.AppendRelativePath(directory_path, &relative_path)) {
    return false;
  }
  relative_path = relative_path.Append(resource_path);
  relative_path = relative_path.NormalizePathSeparators();

  auto entry = path_to_resource_id_.find(relative_path);
  if (entry != path_to_resource_id_.end()) {
    *resource_id = entry->second;
    return true;
  }

  return false;
}

const ui::TemplateReplacements*
ElectronComponentExtensionResourceManager::GetTemplateReplacementsForExtension(
    const std::string& extension_id) const {
  auto it = template_replacements_.find(extension_id);
  if (it == template_replacements_.end()) {
    return nullptr;
  }
  return &it->second;
}

void ElectronComponentExtensionResourceManager::AddComponentResourceEntries(
    const base::span<const webui::ResourcePath> entries) {
  base::FilePath gen_folder_path = base::FilePath().AppendASCII(
      "@out_folder@/gen/chrome/browser/resources/");
  gen_folder_path = gen_folder_path.NormalizePathSeparators();

  for (const auto& entry : entries) {
    const int id = entry.id;
    base::FilePath resource_path = base::FilePath().AppendASCII(entry.path);
    resource_path = resource_path.NormalizePathSeparators();

    if (!gen_folder_path.IsParent(resource_path)) {
      const auto [_, inserted] =
          path_to_resource_id_.try_emplace(std::move(resource_path), id);
      DCHECK(inserted);
    } else {
      // If the resource is a generated file, strip the generated folder's path,
      // so that it can be served from a normal URL (as if it were not
      // generated).
      base::FilePath effective_path =
          base::FilePath().AppendASCII(resource_path.AsUTF8Unsafe().substr(
              gen_folder_path.value().length()));
      const auto [_, inserted] =
          path_to_resource_id_.try_emplace(std::move(effective_path), id);
      DCHECK(inserted);
    }
  }
}

}  // namespace extensions
