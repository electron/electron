// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extensions_browser_api_provider.h"

#include <string>

#include "base/strings/strcat.h"
#include "extensions/browser/api/i18n/i18n_api.h"
#include "extensions/browser/extension_function.h"
#include "extensions/browser/extension_function_histogram_value.h"
#include "extensions/browser/extension_function_registry.h"
#include "shell/browser/extensions/api/extension_action/extension_action_api.h"
#include "shell/browser/extensions/api/generated_api_registration.h"
#include "shell/browser/extensions/api/tabs/tabs_api.h"

namespace extensions {

namespace {

// An ExtensionFunction that always fails. Used to replace core-registered
// functions for APIs that Electron does not support.
class UnsupportedFunction : public ExtensionFunction {
 public:
  UnsupportedFunction() = default;

 protected:
  ~UnsupportedFunction() override = default;

  // ExtensionFunction:
  ResponseAction Run() override {
    return RespondNow(
        Error(base::StrCat({name(), " is not supported in Electron"})));
  }
};

// chrome.webstorePrivate is implemented in //extensions and registered by
// CoreExtensionsBrowserAPIProvider, but every function assumes an
// embedder-provided WebstorePrivateAPIDelegate, which Electron does not have.
// The API is made unavailable to all contexts via the "webstorePrivate" entry
// in shell/common/extensions/api/_api_features.json, so these should never be
// dispatched; replace them anyway so that a request that does reach the
// browser fails with an error rather than dereferencing a null delegate.
constexpr const char* kUnsupportedWebstorePrivateFunctions[] = {
    "webstorePrivate.beginInstallWithManifest3",
    "webstorePrivate.completeInstall",
    "webstorePrivate.enableAppLauncher",
    "webstorePrivate.getBrowserLogin",
    "webstorePrivate.getExtensionStatus",
    "webstorePrivate.getFullChromeVersion",
    "webstorePrivate.getIsLauncherEnabled",
    "webstorePrivate.getMV2DeprecationStatus",
    "webstorePrivate.getReferrerChain",
    "webstorePrivate.getStoreLogin",
    "webstorePrivate.getWebGLStatus",
    "webstorePrivate.isInIncognitoMode",
    "webstorePrivate.isPendingCustodianApproval",
    "webstorePrivate.logEnterprisePromoShown",
    "webstorePrivate.onEnterprisePromoClick",
    "webstorePrivate.setStoreLogin",
    "webstorePrivate.shouldShowEnterprisePromotionBanner",
};

void OverrideUnsupportedFunctions(ExtensionFunctionRegistry* registry) {
  for (const char* name : kUnsupportedWebstorePrivateFunctions) {
    registry->Register(ExtensionFunctionRegistry::FactoryEntry(
        &NewExtensionFunction<UnsupportedFunction>, name, functions::UNKNOWN));
  }
}

}  // namespace

ElectronExtensionsBrowserAPIProvider::ElectronExtensionsBrowserAPIProvider() =
    default;
ElectronExtensionsBrowserAPIProvider::~ElectronExtensionsBrowserAPIProvider() =
    default;

void ElectronExtensionsBrowserAPIProvider::RegisterExtensionFunctions(
    ExtensionFunctionRegistry* registry) {
  // Generated APIs from Electron.
  api::ElectronGeneratedFunctionRegistry::RegisterAll(registry);

  // This provider is added after CoreExtensionsBrowserAPIProvider, so this
  // replaces the core registrations.
  OverrideUnsupportedFunctions(registry);
}

}  // namespace extensions
