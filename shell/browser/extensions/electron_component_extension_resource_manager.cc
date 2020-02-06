// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_component_extension_resource_manager.h"

#include <string>

#include "base/logging.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/grit/component_extension_resources_map.h"

namespace extensions {

ElectronComponentExtensionResourceManager::
    ElectronComponentExtensionResourceManager() {
  AddComponentResourceEntries(kComponentExtensionResources,
                              kComponentExtensionResourcesSize);
}

ElectronComponentExtensionResourceManager::
    ~ElectronComponentExtensionResourceManager() {}

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
  auto it = extension_template_replacements_.find(extension_id);
  if (it == extension_template_replacements_.end()) {
    return nullptr;
  }
  return &it->second;
}

void ElectronComponentExtensionResourceManager::AddComponentResourceEntries(
    const GritResourceMap* entries,
    size_t size) {
  base::FilePath gen_folder_path = base::FilePath().AppendASCII(
      "@out_folder@/gen/chrome/browser/resources/");
  gen_folder_path = gen_folder_path.NormalizePathSeparators();

  for (size_t i = 0; i < size; ++i) {
    base::FilePath resource_path =
        base::FilePath().AppendASCII(entries[i].name);
    resource_path = resource_path.NormalizePathSeparators();

    if (!gen_folder_path.IsParent(resource_path)) {
      DCHECK(!base::Contains(path_to_resource_id_, resource_path));
      path_to_resource_id_[resource_path] = entries[i].value;
    } else {
      // If the resource is a generated file, strip the generated folder's path,
      // so that it can be served from a normal URL (as if it were not
      // generated).
      base::FilePath effective_path =
          base::FilePath().AppendASCII(resource_path.AsUTF8Unsafe().substr(
              gen_folder_path.value().length()));
      DCHECK(!base::Contains(path_to_resource_id_, effective_path));
      path_to_resource_id_[effective_path] = entries[i].value;
    }
  }
}

}  // namespace extensions
