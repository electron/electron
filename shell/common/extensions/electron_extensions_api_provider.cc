// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/common/extensions/electron_extensions_api_provider.h"

#include <string>

#include "electron/buildflags/buildflags.h"
#include "extensions/common/features/json_feature_provider_source.h"

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "shell/common/extensions/api/manifest_features.h"
#endif

namespace electron {

ElectronExtensionsAPIProvider::ElectronExtensionsAPIProvider() = default;
ElectronExtensionsAPIProvider::~ElectronExtensionsAPIProvider() = default;

// TODO(samuelmaddock): generate API features?

void ElectronExtensionsAPIProvider::AddAPIFeatures(
    extensions::FeatureProvider* provider) {
  // AddShellAPIFeatures(provider);
}

void ElectronExtensionsAPIProvider::AddManifestFeatures(
    extensions::FeatureProvider* provider) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // TODO(samuelmaddock): why is the extensions namespace generated?
  extensions::AddAtomManifestFeatures(provider);
#endif
}

void ElectronExtensionsAPIProvider::AddPermissionFeatures(
    extensions::FeatureProvider* provider) {
  // No shell-specific permission features.
}

void ElectronExtensionsAPIProvider::AddBehaviorFeatures(
    extensions::FeatureProvider* provider) {
  // No shell-specific behavior features.
}

void ElectronExtensionsAPIProvider::AddAPIJSONSources(
    extensions::JSONFeatureProviderSource* json_source) {
  // json_source->LoadJSON(IDR_SHELL_EXTENSION_API_FEATURES);
}

bool ElectronExtensionsAPIProvider::IsAPISchemaGenerated(
    const std::string& name) {
  // return shell::api::ShellGeneratedSchemas::IsGenerated(name);
  return false;
}

base::StringPiece ElectronExtensionsAPIProvider::GetAPISchema(
    const std::string& name) {
  // return shell::api::ShellGeneratedSchemas::Get(name);
  return "";
}

void ElectronExtensionsAPIProvider::RegisterPermissions(
    extensions::PermissionsInfo* permissions_info) {}

void ElectronExtensionsAPIProvider::RegisterManifestHandlers() {}

}  // namespace electron
