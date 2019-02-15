// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/synchronization/lock.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_process_host_observer.h"
#include "net/ssl/client_cert_identity.h"

namespace content {
class QuotaPermissionContext;
class ClientCertificateDelegate;
}  // namespace content

namespace net {
class SSLCertRequestInfo;
}

namespace atom {

class AtomResourceDispatcherHostDelegate;
class NotificationPresenter;
class PlatformNotificationService;

class AtomBrowserClient : public content::ContentBrowserClient,
                          public content::RenderProcessHostObserver {
 public:
  static AtomBrowserClient* Get();
  static void SetApplicationLocale(const std::string& locale);

  AtomBrowserClient();
  ~AtomBrowserClient() override;

  using Delegate = content::ContentBrowserClient;
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // Returns the WebContents for pending render processes.
  content::WebContents* GetWebContentsFromProcessID(int process_id);

  // Don't force renderer process to restart for once.
  static void SuppressRendererProcessRestartForOnce();

  NotificationPresenter* GetNotificationPresenter();

  void WebNotificationAllowed(int render_process_id,
                              const base::Callback<void(bool, bool)>& callback);

  // content::NavigatorDelegate
  std::vector<std::unique_ptr<content::NavigationThrottle>>
  CreateThrottlesForNavigation(content::NavigationHandle* handle) override;

  // content::ContentBrowserClient:
  std::string GetApplicationLocale() override;

  // content::ContentBrowserClient:
  bool ShouldEnableStrictSiteIsolation() override;

 protected:
  void RenderProcessWillLaunch(
      content::RenderProcessHost* host,
      service_manager::mojom::ServiceRequest* service_request) override;
  content::SpeechRecognitionManagerDelegate*
  CreateSpeechRecognitionManagerDelegate() override;
  content::TtsControllerDelegate* GetTtsControllerDelegate() override;
  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                           content::WebPreferences* prefs) override;
  SiteInstanceForNavigationType ShouldOverrideSiteInstanceForNavigation(
      content::RenderFrameHost* current_rfh,
      content::RenderFrameHost* speculative_rfh,
      content::BrowserContext* browser_context,
      const GURL& url,
      bool has_request_started,
      content::SiteInstance** affinity_site_instance) const override;
  void RegisterPendingSiteInstance(
      content::RenderFrameHost* render_frame_host,
      content::SiteInstance* pending_site_instance) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  void AdjustUtilityServiceProcessCommandLine(
      const service_manager::Identity& identity,
      base::CommandLine* command_line) override;
  void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
  std::string GetGeolocationApiKey() override;
  content::QuotaPermissionContext* CreateQuotaPermissionContext() override;
  content::GeneratedCodeCacheSettings GetGeneratedCodeCacheSettings(
      content::BrowserContext* context) override;
  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      content::ResourceType resource_type,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(content::CertificateRequestResultType)>&
          callback) override;
  void SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      net::ClientCertIdentityList client_certs,
      std::unique_ptr<content::ClientCertificateDelegate> delegate) override;
  void ResourceDispatcherHostCreated() override;
  bool CanCreateWindow(content::RenderFrameHost* opener,
                       const GURL& opener_url,
                       const GURL& opener_top_level_frame_url,
                       const url::Origin& source_origin,
                       content::mojom::WindowContainerType container_type,
                       const GURL& target_url,
                       const content::Referrer& referrer,
                       const std::string& frame_name,
                       WindowOpenDisposition disposition,
                       const blink::mojom::WindowFeatures& features,
                       const std::vector<std::string>& additional_features,
                       const scoped_refptr<network::ResourceRequestBody>& body,
                       bool user_gesture,
                       bool opener_suppressed,
                       bool* no_javascript_access) override;
  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* additional_schemes) override;
  void GetAdditionalWebUISchemes(
      std::vector<std::string>* additional_schemes) override;
  void SiteInstanceDeleting(content::SiteInstance* site_instance) override;
  std::unique_ptr<net::ClientCertStore> CreateClientCertStore(
      content::ResourceContext* resource_context) override;
  std::unique_ptr<device::LocationProvider> OverrideSystemLocationProvider()
      override;
  network::mojom::NetworkContextPtr CreateNetworkContext(
      content::BrowserContext* browser_context,
      bool in_memory,
      const base::FilePath& relative_partition_path) override;
  void RegisterOutOfProcessServices(OutOfProcessServiceMap* services) override;
  base::Optional<service_manager::Manifest> GetServiceManifestOverlay(
      base::StringPiece name) override;
  net::NetLog* GetNetLog() override;
  content::MediaObserver* GetMediaObserver() override;
  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;
  content::PlatformNotificationService* GetPlatformNotificationService()
      override;
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams&) override;
  base::FilePath GetDefaultDownloadDirectory() override;
  scoped_refptr<network::SharedURLLoaderFactory>
  GetSystemSharedURLLoaderFactory() override;
  void OnNetworkServiceCreated(
      network::mojom::NetworkService* network_service) override;
  bool ShouldBypassCORB(int render_process_id) const override;
  std::string GetProduct() const override;
  std::string GetUserAgent() const override;

  // content::RenderProcessHostObserver:
  void RenderProcessHostDestroyed(content::RenderProcessHost* host) override;
  void RenderProcessReady(content::RenderProcessHost* host) override;
  void RenderProcessExited(
      content::RenderProcessHost* host,
      const content::ChildProcessTerminationInfo& info) override;
  bool HandleExternalProtocol(
      const GURL& url,
      content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
      int child_id,
      content::NavigationUIData* navigation_data,
      bool is_main_frame,
      ui::PageTransition page_transition,
      bool has_user_gesture,
      const std::string& method,
      const net::HttpRequestHeaders& headers) override;

 private:
  struct ProcessPreferences {
    bool sandbox = false;
    bool native_window_open = false;
    bool disable_popups = false;
    bool web_security = true;
  };

  bool ShouldForceNewSiteInstance(content::RenderFrameHost* current_rfh,
                                  content::RenderFrameHost* speculative_rfh,
                                  content::BrowserContext* browser_context,
                                  const GURL& dest_url,
                                  bool has_request_started) const;
  bool NavigationWasRedirectedCrossSite(
      content::BrowserContext* browser_context,
      content::SiteInstance* current_instance,
      content::SiteInstance* speculative_instance,
      const GURL& dest_url,
      bool has_request_started) const;
  void AddProcessPreferences(int process_id, ProcessPreferences prefs);
  void RemoveProcessPreferences(int process_id);
  bool IsProcessObserved(int process_id) const;
  bool IsRendererSandboxed(int process_id) const;
  bool RendererUsesNativeWindowOpen(int process_id) const;
  bool RendererDisablesPopups(int process_id) const;
  std::string GetAffinityPreference(content::RenderFrameHost* rfh) const;
  content::SiteInstance* GetSiteInstanceFromAffinity(
      content::BrowserContext* browser_context,
      const GURL& url,
      content::RenderFrameHost* rfh) const;
  void ConsiderSiteInstanceForAffinity(content::RenderFrameHost* rfh,
                                       content::SiteInstance* site_instance);

  // pending_render_process => web contents.
  std::map<int, content::WebContents*> pending_processes_;

  std::map<int, base::ProcessId> render_process_host_pids_;

  // list of site per affinity. weak_ptr to prevent instance locking
  std::map<std::string, content::SiteInstance*> site_per_affinities_;

  std::unique_ptr<AtomResourceDispatcherHostDelegate>
      resource_dispatcher_host_delegate_;

  std::unique_ptr<PlatformNotificationService> notification_service_;
  std::unique_ptr<NotificationPresenter> notification_presenter_;

  Delegate* delegate_ = nullptr;

  mutable base::Lock process_preferences_lock_;
  std::map<int, ProcessPreferences> process_preferences_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserClient);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
