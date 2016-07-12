// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSIONS_BROWSER_CLIENT_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSIONS_BROWSER_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/extension_host_delegate.h"

namespace base {
class CommandLine;
}

namespace content {
class BrowserContext;
}

namespace extensions {

class AtomProcessManagerDelegate;
class AtomComponentExtensionResourceManager;
class ExtensionsAPIClient;

// Implementation of BrowserClient for Chrome, which includes
// knowledge of Profiles, BrowserContexts and incognito.
//
// NOTE: Methods that do not require knowledge of browser concepts should be
// implemented in ChromeExtensionsClient even if they are only used in the
// browser process (see chrome/common/extensions/chrome_extensions_client.h).
class AtomExtensionsBrowserClient : public ExtensionsBrowserClient {
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
#if defined(OS_CHROMEOS)
  std::string GetUserIdHashFromContext(
      content::BrowserContext* context) override;
#endif
  bool IsGuestSession(content::BrowserContext* context) const override;
  static bool IsIncognitoEnabled(
      const std::string& extension_id,
      content::BrowserContext* context);
  bool IsExtensionIncognitoEnabled(
      const std::string& extension_id,
      content::BrowserContext* context) const override {
    return IsIncognitoEnabled(extension_id, context);
  };
  bool CanExtensionCrossIncognito(
      const Extension* extension,
      content::BrowserContext* context) const override;
  net::URLRequestJob* MaybeCreateResourceBundleRequestJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      const base::FilePath& directory_path,
      const std::string& content_security_policy,
      bool send_cors_header) override;
  bool AllowCrossRendererResourceLoad(net::URLRequest* request,
                                      bool is_incognito,
                                      const Extension* extension,
                                      InfoMap* extension_info_map) override;
  PrefService* GetPrefServiceForContext(
      content::BrowserContext* context) override;
  void GetEarlyExtensionPrefsObservers(
      content::BrowserContext* context,
      std::vector<ExtensionPrefsObserver*>* observers) const override;
  ProcessManagerDelegate* GetProcessManagerDelegate() const override;
  std::unique_ptr<ExtensionHostDelegate> CreateExtensionHostDelegate() override;
  bool DidVersionUpdate(content::BrowserContext* context) override;
  void PermitExternalProtocolHandler() override;
  bool IsRunningInForcedAppMode() override;
  bool IsLoggedInAsPublicAccount() override;
  ApiActivityMonitor* GetApiActivityMonitor(
      content::BrowserContext* context) override;
  ExtensionSystemProvider* GetExtensionSystemFactory() override;
  void RegisterExtensionFunctions(
      ExtensionFunctionRegistry* registry) const override;
  void RegisterMojoServices(content::RenderFrameHost* render_frame_host,
                            const Extension* extension) const override;
  std::unique_ptr<RuntimeAPIDelegate> CreateRuntimeAPIDelegate(
      content::BrowserContext* context) const override;
  const ComponentExtensionResourceManager*
  GetComponentExtensionResourceManager() override;
  void BroadcastEventToRenderers(events::HistogramValue histogram_value,
                                 const std::string& event_name,
                                 std::unique_ptr<base::ListValue>
                                     args) override;
  net::NetLog* GetNetLog() override;
  ExtensionCache* GetExtensionCache() override;
  bool IsBackgroundUpdateAllowed() override;
  bool IsMinBrowserVersionSupported(const std::string& min_version) override;
  ExtensionWebContentsObserver* GetExtensionWebContentsObserver(
      content::WebContents* web_contents) override;
  void CleanUpWebView(content::BrowserContext* browser_context,
                      int embedder_process_id,
                      int view_instance_id) override;
  void AttachExtensionTaskManagerTag(content::WebContents* web_contents,
                                     ViewType view_type) override;
  std::unique_ptr<ExtensionApiFrameIdMapHelper>
  CreateExtensionApiFrameIdMapHelper(
      ExtensionApiFrameIdMap* map) override;

 private:
  friend struct base::DefaultLazyInstanceTraits<AtomExtensionsBrowserClient>;

  // Not owned.
  content::BrowserContext* browser_context_;

    // Support for ProcessManager.
  std::unique_ptr<AtomProcessManagerDelegate> process_manager_delegate_;

  // Client for API implementations.
  std::unique_ptr<ExtensionsAPIClient> api_client_;

  std::unique_ptr<AtomComponentExtensionResourceManager> resource_manager_;

  std::unique_ptr<ExtensionCache> extension_cache_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsBrowserClient);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSIONS_BROWSER_CLIENT_H_
