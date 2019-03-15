// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_ATOM_COMMON_ATOM_EXTENSIONS_CLIENT_H_
#define EXTENSIONS_ATOM_COMMON_ATOM_EXTENSIONS_CLIENT_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "extensions/common/extensions_client.h"
#include "url/gurl.h"

namespace atom {

// The app_shell implementation of ExtensionsClient.
class AtomExtensionsClient : public extensions::ExtensionsClient {
 public:
  AtomExtensionsClient();
  ~AtomExtensionsClient() override;

  // ExtensionsClient overrides:
  void Initialize() override;
  void InitializeWebStoreUrls(base::CommandLine* command_line) override;
  const PermissionMessageProvider& GetPermissionMessageProvider()
      const override;
  const std::string GetProductName() override;
  void FilterHostPermissions(const URLPatternSet& hosts,
                             URLPatternSet* new_hosts,
                             PermissionIDSet* permissions) const override;
  void SetScriptingWhitelist(const ScriptingWhitelist& whitelist) override;
  const ScriptingWhitelist& GetScriptingWhitelist() const override;
  URLPatternSet GetPermittedChromeSchemeHosts(
      const Extension* extension,
      const APIPermissionSet& api_permissions) const override;
  bool IsScriptableURL(const GURL& url, std::string* error) const override;
  bool ShouldSuppressFatalErrors() const override;
  void RecordDidSuppressFatalError() override;
  const GURL& GetWebstoreBaseURL() const override;
  const GURL& GetWebstoreUpdateURL() const override;
  bool IsBlacklistUpdateURL(const GURL& url) const override;

 private:
  ScriptingWhitelist scripting_whitelist_;

  const GURL webstore_base_url_;
  const GURL webstore_update_url_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsClient);
};

}  // namespace atom

#endif  // EXTENSIONS_ATOM_COMMON_ATOM_EXTENSIONS_CLIENT_H_
