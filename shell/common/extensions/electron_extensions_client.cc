// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/common/extensions/electron_extensions_client.h"

#include <memory>
#include <string>

#include "base/no_destructor.h"
#include "components/version_info/version_info.h"
#include "content/public/common/user_agent.h"
#include "extensions/common/core_extensions_api_provider.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/features/simple_feature.h"
#include "extensions/common/permissions/permission_message_provider.h"
#include "extensions/common/url_pattern_set.h"
#include "shell/common/extensions/electron_extensions_api_provider.h"

using extensions::ExtensionsClient;

namespace electron {

namespace {

// TODO(jamescook): Refactor ChromePermissionsMessageProvider so we can share
// code. For now, this implementation does nothing.
class ElectronPermissionMessageProvider
    : public extensions::PermissionMessageProvider {
 public:
  ElectronPermissionMessageProvider() = default;
  ~ElectronPermissionMessageProvider() override = default;

  // disable copy
  ElectronPermissionMessageProvider(const ElectronPermissionMessageProvider&) =
      delete;
  ElectronPermissionMessageProvider& operator=(
      const ElectronPermissionMessageProvider&) = delete;

  // PermissionMessageProvider implementation.
  extensions::PermissionMessages GetPermissionMessages(
      const extensions::PermissionIDSet& permissions) const override {
    return extensions::PermissionMessages();
  }

  bool IsPrivilegeIncrease(
      const extensions::PermissionSet& granted_permissions,
      const extensions::PermissionSet& requested_permissions,
      extensions::Manifest::Type extension_type) const override {
    // Ensure we implement this before shipping.
    CHECK(false);
    return false;
  }

  extensions::PermissionIDSet GetAllPermissionIDs(
      const extensions::PermissionSet& permissions,
      extensions::Manifest::Type extension_type) const override {
    return extensions::PermissionIDSet();
  }
};

}  // namespace

ElectronExtensionsClient::ElectronExtensionsClient()
    : webstore_base_url_(extension_urls::kChromeWebstoreBaseURL),
      new_webstore_base_url_(extension_urls::kNewChromeWebstoreBaseURL),
      webstore_update_url_(extension_urls::kChromeWebstoreUpdateURL) {
  AddAPIProvider(std::make_unique<extensions::CoreExtensionsAPIProvider>());
  AddAPIProvider(std::make_unique<ElectronExtensionsAPIProvider>());
}

ElectronExtensionsClient::~ElectronExtensionsClient() = default;

void ElectronExtensionsClient::Initialize() {
  // TODO(jamescook): Do we need to whitelist any extensions?
}

void ElectronExtensionsClient::InitializeWebStoreUrls(
    base::CommandLine* command_line) {}

const extensions::PermissionMessageProvider&
ElectronExtensionsClient::GetPermissionMessageProvider() const {
  NOTIMPLEMENTED();

  static base::NoDestructor<ElectronPermissionMessageProvider> instance;
  return *instance;
}

const std::string ElectronExtensionsClient::GetProductName() {
  // TODO(samuelmaddock):
  return "app_shell";
}

void ElectronExtensionsClient::FilterHostPermissions(
    const extensions::URLPatternSet& hosts,
    extensions::URLPatternSet* new_hosts,
    extensions::PermissionIDSet* permissions) const {
  NOTIMPLEMENTED();
}

void ElectronExtensionsClient::SetScriptingAllowlist(
    const ExtensionsClient::ScriptingAllowlist& allowlist) {
  scripting_allowlist_ = allowlist;
}

const ExtensionsClient::ScriptingAllowlist&
ElectronExtensionsClient::GetScriptingAllowlist() const {
  // TODO(jamescook): Real whitelist.
  return scripting_allowlist_;
}

extensions::URLPatternSet
ElectronExtensionsClient::GetPermittedChromeSchemeHosts(
    const extensions::Extension* extension,
    const extensions::APIPermissionSet& api_permissions) const {
  return extensions::URLPatternSet();
}

bool ElectronExtensionsClient::IsScriptableURL(const GURL& url,
                                               std::string* error) const {
  // No restrictions on URLs.
  return true;
}

const GURL& ElectronExtensionsClient::GetWebstoreBaseURL() const {
  return webstore_base_url_;
}

const GURL& ElectronExtensionsClient::GetNewWebstoreBaseURL() const {
  return new_webstore_base_url_;
}

const GURL& ElectronExtensionsClient::GetWebstoreUpdateURL() const {
  return webstore_update_url_;
}

bool ElectronExtensionsClient::IsBlocklistUpdateURL(const GURL& url) const {
  return false;
}

}  // namespace electron
