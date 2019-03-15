// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_ATOM_COMMON_ATOM_EXTENSIONS_API_PROVIDER_H_
#define EXTENSIONS_ATOM_COMMON_ATOM_EXTENSIONS_API_PROVIDER_H_

#include "base/macros.h"
#include "extensions/common/extensions_api_provider.h"

namespace atom {

class AtomExtensionsAPIProvider : public ExtensionsAPIProvider {
 public:
  AtomExtensionsAPIProvider();
  ~AtomExtensionsAPIProvider() override;

  // ExtensionsAPIProvider:
  void AddAPIFeatures(extensions::FeatureProvider* provider) override;
  void AddManifestFeatures(extensions::FeatureProvider* provider) override;
  void AddPermissionFeatures(extensions::FeatureProvider* provider) override;
  void AddBehaviorFeatures(extensions::FeatureProvider* provider) override;
  void AddAPIJSONSources(
      extensions::JSONFeatureProviderSource* json_source) override;
  bool IsAPISchemaGenerated(const std::string& name) override;
  base::StringPiece GetAPISchema(const std::string& name) override;
  void RegisterPermissions(PermissionsInfo* permissions_info) override;
  void RegisterManifestHandlers() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsAPIProvider);
};

}  // namespace atom

#endif  // EXTENSIONS_ATOM_COMMON_ATOM_EXTENSIONS_API_PROVIDER_H_
