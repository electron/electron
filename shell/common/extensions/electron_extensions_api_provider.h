// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_API_PROVIDER_H_
#define SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_API_PROVIDER_H_

#include <string>

#include "base/macros.h"
#include "extensions/common/extensions_api_provider.h"

namespace electron {

class ElectronExtensionsAPIProvider : public extensions::ExtensionsAPIProvider {
 public:
  ElectronExtensionsAPIProvider();
  ~ElectronExtensionsAPIProvider() override;

  // ExtensionsAPIProvider:
  void AddAPIFeatures(extensions::FeatureProvider* provider) override;
  void AddManifestFeatures(extensions::FeatureProvider* provider) override;
  void AddPermissionFeatures(extensions::FeatureProvider* provider) override;
  void AddBehaviorFeatures(extensions::FeatureProvider* provider) override;
  void AddAPIJSONSources(
      extensions::JSONFeatureProviderSource* json_source) override;
  bool IsAPISchemaGenerated(const std::string& name) override;
  base::StringPiece GetAPISchema(const std::string& name) override;
  void RegisterPermissions(
      extensions::PermissionsInfo* permissions_info) override;
  void RegisterManifestHandlers() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionsAPIProvider);
};

}  // namespace electron

#endif  // SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_API_PROVIDER_H_
