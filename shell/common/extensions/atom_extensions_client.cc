// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/common/extensions/atom_extensions_client.h"

#include <memory>
#include <string>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "components/version_info/version_info.h"
#include "content/public/common/user_agent.h"
#include "extensions/common/core_extensions_api_provider.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/features/simple_feature.h"
#include "extensions/common/permissions/permission_message_provider.h"
#include "extensions/common/url_pattern_set.h"
#include "shell/common/extensions/atom_extensions_api_provider.h"

using extensions::ExtensionsClient;

namespace electron {

namespace {

// TODO(jamescook): Refactor ChromePermissionsMessageProvider so we can share
// code. For now, this implementation does nothing.
class AtomPermissionMessageProvider
    : public extensions::PermissionMessageProvider {
 public:
  AtomPermissionMessageProvider() {}
  ~AtomPermissionMessageProvider() override {}

  // PermissionMessageProvider implementation.
  extensions::PermissionMessages GetPermissionMessages(
      const extensions::PermissionIDSet& permissions) const override {
    return extensions::PermissionMessages();
  }

  extensions::PermissionMessages GetPowerfulPermissionMessages(
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

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomPermissionMessageProvider);
};

base::LazyInstance<AtomPermissionMessageProvider>::DestructorAtExit
    g_permission_message_provider = LAZY_INSTANCE_INITIALIZER;

}  // namespace

AtomExtensionsClient::AtomExtensionsClient()
    : webstore_base_url_(extension_urls::kChromeWebstoreBaseURL),
      webstore_update_url_(extension_urls::kChromeWebstoreUpdateURL) {
  AddAPIProvider(std::make_unique<extensions::CoreExtensionsAPIProvider>());
  AddAPIProvider(std::make_unique<AtomExtensionsAPIProvider>());
}

AtomExtensionsClient::~AtomExtensionsClient() {}

void AtomExtensionsClient::Initialize() {
  // TODO(jamescook): Do we need to whitelist any extensions?
}

void AtomExtensionsClient::InitializeWebStoreUrls(
    base::CommandLine* command_line) {}

const extensions::PermissionMessageProvider&
AtomExtensionsClient::GetPermissionMessageProvider() const {
  NOTIMPLEMENTED();
  return g_permission_message_provider.Get();
}

const std::string AtomExtensionsClient::GetProductName() {
  // TODO(samuelmaddock):
  return "app_shell";
}

void AtomExtensionsClient::FilterHostPermissions(
    const extensions::URLPatternSet& hosts,
    extensions::URLPatternSet* new_hosts,
    extensions::PermissionIDSet* permissions) const {
  NOTIMPLEMENTED();
}

void AtomExtensionsClient::SetScriptingWhitelist(
    const ExtensionsClient::ScriptingWhitelist& whitelist) {
  scripting_whitelist_ = whitelist;
}

const ExtensionsClient::ScriptingWhitelist&
AtomExtensionsClient::GetScriptingWhitelist() const {
  // TODO(jamescook): Real whitelist.
  return scripting_whitelist_;
}

extensions::URLPatternSet AtomExtensionsClient::GetPermittedChromeSchemeHosts(
    const extensions::Extension* extension,
    const extensions::APIPermissionSet& api_permissions) const {
  NOTIMPLEMENTED();
  return extensions::URLPatternSet();
}

bool AtomExtensionsClient::IsScriptableURL(const GURL& url,
                                           std::string* error) const {
  // No restrictions on URLs.
  return true;
}

const GURL& AtomExtensionsClient::GetWebstoreBaseURL() const {
  return webstore_base_url_;
}

const GURL& AtomExtensionsClient::GetWebstoreUpdateURL() const {
  return webstore_update_url_;
}

bool AtomExtensionsClient::IsBlacklistUpdateURL(const GURL& url) const {
  // TODO(rockot): Maybe we want to do something else here. For now we accept
  // any URL as a blacklist URL because we don't really care.
  return true;
}

}  // namespace electron
