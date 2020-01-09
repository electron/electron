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
#include "extensions/common/alias.h"
#include "extensions/common/features/feature_provider.h"
#include "extensions/common/features/json_feature_provider_source.h"
#include "extensions/common/features/simple_feature.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handler.h"
#include "extensions/common/manifest_handlers/permissions_parser.h"
#include "extensions/common/manifest_url_handlers.h"
#include "extensions/common/permissions/permissions_info.h"
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

// TODO(samuelmaddock): generate API features?

void AtomExtensionsAPIProvider::AddAPIFeatures(
    extensions::FeatureProvider* provider) {
  // AddShellAPIFeatures(provider);

  {
    extensions::SimpleFeature* feature = new extensions::SimpleFeature();
    feature->set_name("tabs");
    // feature->set_channel(extensions::version_info::Channel::STABLE);
    feature->set_contexts({extensions::Feature::BLESSED_EXTENSION_CONTEXT});
    feature->set_extension_types({extensions::Manifest::TYPE_EXTENSION});
    provider->AddFeature("tabs", feature);
  }
}

void AtomExtensionsAPIProvider::AddManifestFeatures(
    extensions::FeatureProvider* provider) {
  // TODO(samuelmaddock): why is the extensions namespace generated?
  extensions::AddAtomManifestFeatures(provider);
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
  if (name == "tabs") {
    return true;
  }
  // return shell::api::ShellGeneratedSchemas::IsGenerated(name);
  return false;
}

base::StringPiece AtomExtensionsAPIProvider::GetAPISchema(
    const std::string& name) {
  if (name == "tabs") {
    return "{\"namespace\": \"tabs\", \"functions\": [{\"name\": "
           "\"executeScript\", \"type\": \"function\", \"parameters\": "
           "[{\"type\": \"integer\", \"name\": \"tabId\", \"minimum\": 0, "
           "\"optional\": true, \"description\": \"The ID of the tab in which "
           "to run the script; defaults to the active tab of the current "
           "window.\"}, { \"$ref\": \"extensionTypes.InjectDetails\", "
           "\"name\": \"details\", \"description\": \"Details of the script to "
           "run. Either the code or the file property must be set, but both "
           "may not be set at the same time.\" }, { \"type\": \"function\", "
           "\"name\": \"callback\", \"optional\": true, \"description\": "
           "\"Called after all the JavaScript has been executed.\", "
           "\"parameters\": [ { \"name\": \"result\", \"optional\": true, "
           "\"type\": \"array\", \"items\": {\"type\": \"any\", \"minimum\": "
           "0}, \"description\": \"The result of the script in every injected "
           "frame.\" } ] } ]}]}";
  }
  // return shell::api::ShellGeneratedSchemas::Get(name);
  return "";
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
