// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_CLIENT_H_
#define ELECTRON_SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_CLIENT_H_

#include <string>

#include "extensions/common/extensions_client.h"
#include "url/gurl.h"

namespace extensions {
class APIPermissionSet;
class Extension;
class PermissionMessageProvider;
class PermissionIDSet;
class URLPatternSet;
}  // namespace extensions

namespace electron {

// The app_shell implementation of ExtensionsClient.
class ElectronExtensionsClient : public extensions::ExtensionsClient {
 public:
  using ScriptingAllowlist = extensions::ExtensionsClient::ScriptingAllowlist;

  ElectronExtensionsClient();
  ~ElectronExtensionsClient() override;

  // disable copy
  ElectronExtensionsClient(const ElectronExtensionsClient&) = delete;
  ElectronExtensionsClient& operator=(const ElectronExtensionsClient&) = delete;

  // ExtensionsClient overrides:
  void Initialize() override;
  void InitializeWebStoreUrls(base::CommandLine* command_line) override;
  const extensions::PermissionMessageProvider& GetPermissionMessageProvider()
      const override;
  const std::string GetProductName() override;
  void FilterHostPermissions(
      const extensions::URLPatternSet& hosts,
      extensions::URLPatternSet* new_hosts,
      extensions::PermissionIDSet* permissions) const override;
  void SetScriptingAllowlist(const ScriptingAllowlist& allowlist) override;
  const ScriptingAllowlist& GetScriptingAllowlist() const override;
  extensions::URLPatternSet GetPermittedChromeSchemeHosts(
      const extensions::Extension* extension,
      const extensions::APIPermissionSet& api_permissions) const override;
  bool IsScriptableURL(const GURL& url, std::string* error) const override;
  const GURL& GetWebstoreBaseURL() const override;
  const GURL& GetWebstoreUpdateURL() const override;
  bool IsBlocklistUpdateURL(const GURL& url) const override;
  const GURL& GetNewWebstoreBaseURL() const override;

 private:
  ScriptingAllowlist scripting_allowlist_;

  const GURL webstore_base_url_;
  const GURL new_webstore_base_url_;
  const GURL webstore_update_url_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_CLIENT_H_
