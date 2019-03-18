// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_ATOM_BROWSER_ATOM_EXTENSIONS_BROWSER_CLIENT_H_
#define EXTENSIONS_ATOM_BROWSER_ATOM_EXTENSIONS_BROWSER_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "extensions/browser/extensions_browser_client.h"

class PrefService;

namespace extensions {
class ExtensionsAPIClient;
class KioskDelegate;
class ProcessManagerDelegate;
class ProcessMap;
}  // namespace extensions

namespace atom {

// An ExtensionsBrowserClient that supports a single content::BrowserContext
// with no related incognito context.
// Must be initialized via InitWithBrowserContext() once the BrowserContext is
// created.
class AtomExtensionsBrowserClient : public extensions::ExtensionsBrowserClient {
 public:
  AtomExtensionsBrowserClient();
  ~AtomExtensionsBrowserClient() override;

  // ExtensionsBrowserClient overrides:
  bool IsShuttingDown() override;
  bool AreExtensionsDisabled(const base::CommandLine& command_line,
                             content::BrowserContext* context) override;
  bool IsValidContext(content::BrowserContext* context) override;
  bool IsSameContext(content::BrowserContext* first,
                     content::BrowserContext* second) override;
  bool HasOffTheRecordContext(content::BrowserContext* context) override;
  content::BrowserContext* GetOffTheRecordContext(
      content::BrowserContext* context) override;
  content::BrowserContext* GetOriginalContext(
      content::BrowserContext* context) override;
  bool IsGuestSession(content::BrowserContext* context) const override;
  bool IsExtensionIncognitoEnabled(
      const std::string& extension_id,
      content::BrowserContext* context) const override;
  bool CanExtensionCrossIncognito(
      const extensions::Extension* extension,
      content::BrowserContext* context) const override;
  net::URLRequestJob* MaybeCreateResourceBundleRequestJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      const base::FilePath& directory_path,
      const std::string& content_security_policy,
      bool send_cors_header) override;
  base::FilePath GetBundleResourcePath(
      const network::ResourceRequest& request,
      const base::FilePath& extension_resources_path,
      extensions::ComponentExtensionResourceInfo* resource_info) const override;
  void LoadResourceFromResourceBundle(
      const network::ResourceRequest& request,
      network::mojom::URLLoaderRequest loader,
      const base::FilePath& resource_relative_path,
      const extensions::ComponentExtensionResourceInfo& resource_info,
      const std::string& content_security_policy,
      network::mojom::URLLoaderClientPtr client,
      bool send_cors_header) override;
  bool AllowCrossRendererResourceLoad(
      const GURL& url,
      content::ResourceType resource_type,
      ui::PageTransition page_transition,
      int child_id,
      bool is_incognito,
      const extensions::Extension* extension,
      const extensions::ExtensionSet& extensions,
      const extensions::ProcessMap& process_map) override;
  PrefService* GetPrefServiceForContext(
      content::BrowserContext* context) override;
  void GetEarlyExtensionPrefsObservers(
      content::BrowserContext* context,
      std::vector<extensions::ExtensionPrefsObserver*>* observers)
      const override;
  extensions::ProcessManagerDelegate* GetProcessManagerDelegate()
      const override;
  std::unique_ptr<extensions::ExtensionHostDelegate>
  CreateExtensionHostDelegate() override;
  bool DidVersionUpdate(content::BrowserContext* context) override;
  void PermitExternalProtocolHandler() override;
  bool IsInDemoMode() override;
  bool IsScreensaverInDemoMode(const std::string& app_id) override;
  bool IsRunningInForcedAppMode() override;
  bool IsAppModeForcedForApp(
      const extensions::ExtensionId& extension_id) override;
  bool IsLoggedInAsPublicAccount() override;
  extensions::ExtensionSystemProvider* GetExtensionSystemFactory() override;
  void RegisterExtensionInterfaces(
      service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>*
          registry,
      content::RenderFrameHost* render_frame_host,
      const extensions::Extension* extension) const override;
  std::unique_ptr<extensions::RuntimeAPIDelegate> CreateRuntimeAPIDelegate(
      content::BrowserContext* context) const override;
  const extensions::ComponentExtensionResourceManager*
  GetComponentExtensionResourceManager() override;
  void BroadcastEventToRenderers(
      extensions::events::HistogramValue histogram_value,
      const std::string& event_name,
      std::unique_ptr<base::ListValue> args) override;
  net::NetLog* GetNetLog() override;
  extensions::ExtensionCache* GetExtensionCache() override;
  bool IsBackgroundUpdateAllowed() override;
  bool IsMinBrowserVersionSupported(const std::string& min_version) override;
  extensions::ExtensionWebContentsObserver* GetExtensionWebContentsObserver(
      content::WebContents* web_contents) override;
  extensions::ExtensionNavigationUIData* GetExtensionNavigationUIData(
      net::URLRequest* request) override;
  extensions::KioskDelegate* GetKioskDelegate() override;
  bool IsLockScreenContext(content::BrowserContext* context) override;
  std::string GetApplicationLocale() override;
  std::string GetUserAgent() const override;

  // |context| is the single BrowserContext used for IsValidContext().
  // |pref_service| is used for GetPrefServiceForContext().
  void InitWithBrowserContext(content::BrowserContext* context,
                              PrefService* pref_service);

  // Sets the API client.
  void SetAPIClientForTest(extensions::ExtensionsAPIClient* api_client);

 private:
  // The single BrowserContext for app_shell. Not owned. Must be initialized
  // when ready by calling InitWithBrowserContext().
  content::BrowserContext* browser_context_ = nullptr;

  // The PrefService for |browser_context_|. Not owned. Must be initialized when
  // ready by calling InitWithBrowserContext().
  PrefService* pref_service_ = nullptr;

  // Support for extension APIs.
  std::unique_ptr<extensions::ExtensionsAPIClient> api_client_;

  // The extension cache used for download and installation.
  std::unique_ptr<extensions::ExtensionCache> extension_cache_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsBrowserClient);
};

}  // namespace atom

#endif  // EXTENSIONS_ATOM_BROWSER_ATOM_EXTENSIONS_BROWSER_CLIENT_H_
