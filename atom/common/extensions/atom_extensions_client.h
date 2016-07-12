// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_EXTENSIONS_ATOM_EXTENSIONS_CLIENT_H_
#define ATOM_COMMON_EXTENSIONS_ATOM_EXTENSIONS_CLIENT_H_

#include <string>

#include "atom/common/extensions/permissions/chrome_api_permissions.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "extensions/common/extensions_client.h"
#include "extensions/common/permissions/extensions_api_permissions.h"

namespace extensions {

class AtomExtensionsClient : public ExtensionsClient {
 public:
  AtomExtensionsClient();
  ~AtomExtensionsClient() override;

  void Initialize() override;

  const PermissionMessageProvider& GetPermissionMessageProvider()
      const override;
  const std::string GetProductName() override;
  std::unique_ptr<FeatureProvider> CreateFeatureProvider(
      const std::string& name) const override;
  std::unique_ptr<JSONFeatureProviderSource> CreateFeatureProviderSource(
      const std::string& name) const override;
  void FilterHostPermissions(const URLPatternSet& hosts,
                             URLPatternSet* new_hosts,
                             PermissionIDSet* permissions) const override;
  void SetScriptingWhitelist(const ScriptingWhitelist& whitelist) override;
  const ScriptingWhitelist& GetScriptingWhitelist() const override;
  URLPatternSet GetPermittedChromeSchemeHosts(
      const Extension* extension,
      const APIPermissionSet& api_permissions) const override;
  bool IsScriptableURL(const GURL& url, std::string* error) const override;
  bool IsAPISchemaGenerated(const std::string& name) const override;
  base::StringPiece GetAPISchema(const std::string& name) const override;
  void RegisterAPISchemaResources(ExtensionAPI* api) const override;
  bool ShouldSuppressFatalErrors() const override;
  void RecordDidSuppressFatalError() override;
  std::string GetWebstoreBaseURL() const override;
  std::string GetWebstoreUpdateURL() const override;
  bool IsBlacklistUpdateURL(const GURL& url) const override;

  // Get the LazyInstance for AtomExtensionsClient.
  static AtomExtensionsClient* GetInstance();

 private:
  const ChromeAPIPermissions chrome_api_permissions_;
  const ExtensionsAPIPermissions extensions_api_permissions_;

  ScriptingWhitelist scripting_whitelist_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsClient);
};

}  // namespace extensions

#endif  // ATOM_COMMON_EXTENSIONS_ATOM_EXTENSIONS_CLIENT_H_
