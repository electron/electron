// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/common/extensions/atom_extensions_api_provider.h"

#include <memory>
#include <string>
#include <utility>

#include "base/containers/span.h"
#include "base/strings/utf_string_conversions.h"
#include "electron/buildflags/buildflags.h"
#include "electron/shell/common/extensions/api/generated_schemas.h"
#include "extensions/common/alias.h"
#include "extensions/common/features/feature_provider.h"
#include "extensions/common/features/json_feature_provider_source.h"
#include "extensions/common/features/simple_feature.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handler.h"
#include "extensions/common/manifest_handlers/permissions_parser.h"
#include "extensions/common/manifest_url_handlers.h"
#include "extensions/common/permissions/permissions_info.h"
#include "shell/common/extensions/api/api_features.h"
#include "shell/common/extensions/api/manifest_features.h"

namespace extensions {

namespace keys = manifest_keys;
namespace errors = manifest_errors;

// Parses the "devtools_page" manifest key.
class DevToolsPageHandler : public ManifestHandler {
 public:
  DevToolsPageHandler() = default;
  ~DevToolsPageHandler() override = default;

  bool Parse(Extension* extension, base::string16* error) override {
    std::unique_ptr<ManifestURL> manifest_url(new ManifestURL);
    std::string devtools_str;
    if (!extension->manifest()->GetString(keys::kDevToolsPage, &devtools_str)) {
      *error = base::ASCIIToUTF16(errors::kInvalidDevToolsPage);
      return false;
    }
    manifest_url->url_ = extension->GetResourceURL(devtools_str);
    extension->SetManifestData(keys::kDevToolsPage, std::move(manifest_url));
    PermissionsParser::AddAPIPermission(extension, APIPermission::kDevtools);
    return true;
  }

 private:
  base::span<const char* const> Keys() const override {
    static constexpr const char* kKeys[] = {keys::kDevToolsPage};
    return kKeys;
  }

  DISALLOW_COPY_AND_ASSIGN(DevToolsPageHandler);
};

constexpr APIPermissionInfo::InitInfo permissions_to_register[] = {
    {APIPermission::kDevtools, "devtools",
     APIPermissionInfo::kFlagImpliesFullURLAccess |
         APIPermissionInfo::kFlagCannotBeOptional |
         APIPermissionInfo::kFlagInternal},
};
base::span<const APIPermissionInfo::InitInfo> GetPermissionInfos() {
  return base::make_span(permissions_to_register);
}
base::span<const Alias> GetPermissionAliases() {
  return base::span<const Alias>();
}

}  // namespace extensions

namespace electron {

AtomExtensionsAPIProvider::AtomExtensionsAPIProvider() = default;
AtomExtensionsAPIProvider::~AtomExtensionsAPIProvider() = default;

void AtomExtensionsAPIProvider::AddAPIFeatures(
    extensions::FeatureProvider* provider) {
  extensions::AddElectronAPIFeatures(provider);
}

void AtomExtensionsAPIProvider::AddManifestFeatures(
    extensions::FeatureProvider* provider) {
  extensions::AddElectronManifestFeatures(provider);
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
  return extensions::api::ElectronGeneratedSchemas::IsGenerated(name);
}

base::StringPiece AtomExtensionsAPIProvider::GetAPISchema(
    const std::string& name) {
  return extensions::api::ElectronGeneratedSchemas::Get(name);
}

void AtomExtensionsAPIProvider::RegisterPermissions(
    extensions::PermissionsInfo* permissions_info) {
  permissions_info->RegisterPermissions(extensions::GetPermissionInfos(),
                                        extensions::GetPermissionAliases());
}

void AtomExtensionsAPIProvider::RegisterManifestHandlers() {
  extensions::ManifestHandlerRegistry* registry =
      extensions::ManifestHandlerRegistry::Get();
  registry->RegisterHandler(
      std::make_unique<extensions::DevToolsPageHandler>());
}

}  // namespace electron
