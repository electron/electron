// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/common/extensions/atom_extensions_api_provider.h"

#include <string>

#include "electron/buildflags/buildflags.h"
#include "extensions/common/features/json_feature_provider_source.h"

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "shell/common/extensions/api/manifest_features.h"
#endif

namespace electron {

AtomExtensionsAPIProvider::AtomExtensionsAPIProvider() = default;
AtomExtensionsAPIProvider::~AtomExtensionsAPIProvider() = default;

// TODO(samuelmaddock): generate API features?

void AtomExtensionsAPIProvider::AddAPIFeatures(
    extensions::FeatureProvider* provider) {
  // AddShellAPIFeatures(provider);
}

void AtomExtensionsAPIProvider::AddManifestFeatures(
    extensions::FeatureProvider* provider) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // TODO(samuelmaddock): why is the extensions namespace generated?
  extensions::AddAtomManifestFeatures(provider);
#endif
}

void AtomExtensionsAPIProvider::AddPermissionFeatures(
    extensions::FeatureProvider* provider) {
  // No shell-specific permission features.
}

void AtomExtensionsAPIProvider::AddBehaviorFeatures(
    extensions::FeatureProvider* provider) {
  // No shell-specific behavior features.
}

void AtomExtensionsAPIProvider::AddAPIJSONSources(
    extensions::JSONFeatureProviderSource* json_source) {
  // json_source->LoadJSON(IDR_SHELL_EXTENSION_API_FEATURES);
}

bool AtomExtensionsAPIProvider::IsAPISchemaGenerated(const std::string& name) {
  // return shell::api::ShellGeneratedSchemas::IsGenerated(name);
  return false;
}

base::StringPiece AtomExtensionsAPIProvider::GetAPISchema(
    const std::string& name) {
  // return shell::api::ShellGeneratedSchemas::Get(name);
  return "";
}

void AtomExtensionsAPIProvider::RegisterPermissions(
    extensions::PermissionsInfo* permissions_info) {}

void AtomExtensionsAPIProvider::RegisterManifestHandlers() {}

}  // namespace electron
