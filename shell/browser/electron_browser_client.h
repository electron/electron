// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_BROWSER_CLIENT_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_BROWSER_CLIENT_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_process_host_observer.h"
#include "content/public/browser/web_contents.h"
#include "electron/buildflags/buildflags.h"
#include "net/ssl/client_cert_identity.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "shell/browser/bluetooth/electron_bluetooth_delegate.h"
#include "shell/browser/hid/electron_hid_delegate.h"
#include "shell/browser/serial/electron_serial_delegate.h"
#include "third_party/blink/public/mojom/badging/badging.mojom-forward.h"
#include "third_party/blink/public/mojom/loader/transferrable_url_loader.mojom.h"

namespace content {
class ClientCertificateDelegate;
class QuotaPermissionContext;
}  // namespace content

namespace net {
class SSLCertRequestInfo;
}

namespace electron {

class ElectronBrowserMainParts;
class NotificationPresenter;
class PlatformNotificationService;

class ElectronBrowserClient : public content::ContentBrowserClient,
                              public content::RenderProcessHostObserver {
 public:
  static ElectronBrowserClient* Get();
  static void SetApplicationLocale(const std::string& locale);

  ElectronBrowserClient();
  ~ElectronBrowserClient() override;

  // disable copy
  ElectronBrowserClient(const ElectronBrowserClient&) = delete;
  ElectronBrowserClient& operator=(const ElectronBrowserClient&) = delete;

  using Delegate = content::ContentBrowserClient;
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // Returns the WebContents for pending render processes.
  content::WebContents* GetWebContentsFromProcessID(int process_id);

  NotificationPresenter* GetNotificationPresenter();

  void WebNotificationAllowed(content::RenderFrameHost* rfh,
                              base::OnceCallback<void(bool, bool)> callback);

  // content::NavigatorDelegate
  std::vector<std::unique_ptr<content::NavigationThrottle>>
  CreateThrottlesForNavigation(content::NavigationHandle* handle) override;

  // content::ContentBrowserClient:
  std::string GetApplicationLocale() override;
  base::FilePath GetFontLookupTableCacheDir() override;
  bool ShouldEnableStrictSiteIsolation() override;
  void BindHostReceiverForRenderer(
      content::RenderProcessHost* render_process_host,
      mojo::GenericPendingReceiver receiver) override;
  void ExposeInterfacesToRenderer(
      service_manager::BinderRegistry* registry,
      blink::AssociatedInterfaceRegistry* associated_registry,
      content::RenderProcessHost* render_process_host) override;
  void RegisterBrowserInterfaceBindersForFrame(
      content::RenderFrameHost* render_frame_host,
      mojo::BinderMapWithContext<content::RenderFrameHost*>* map) override;
  void RegisterBrowserInterfaceBindersForServiceWorker(
      mojo::BinderMapWithContext<const content::ServiceWorkerVersionBaseInfo&>*
          map) override;
#if BUILDFLAG(IS_LINUX)
  void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      content::PosixFileDescriptorInfo* mappings) override;
#endif

  std::string GetUserAgent() override;
  void SetUserAgent(const std::string& user_agent);

  content::SerialDelegate* GetSerialDelegate() override;

  content::BluetoothDelegate* GetBluetoothDelegate() override;

  content::HidDelegate* GetHidDelegate() override;

  device::GeolocationManager* GetGeolocationManager() override;

  content::PlatformNotificationService* GetPlatformNotificationService();

 protected:
  void RenderProcessWillLaunch(content::RenderProcessHost* host) override;
  content::SpeechRecognitionManagerDelegate*
  CreateSpeechRecognitionManagerDelegate() override;
  content::TtsPlatform* GetTtsPlatform() override;

  void OverrideWebkitPrefs(content::WebContents* web_contents,
                           blink::web_pref::WebPreferences* prefs) override;
  void RegisterPendingSiteInstance(
      content::RenderFrameHost* render_frame_host,
      content::SiteInstance* pending_site_instance) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
  std::string GetGeolocationApiKey() override;
  scoped_refptr<content::QuotaPermissionContext> CreateQuotaPermissionContext()
      override;
  content::GeneratedCodeCacheSettings GetGeneratedCodeCacheSettings(
      content::BrowserContext* context) override;
  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      bool is_main_frame_request,
      bool strict_enforcement,
      base::OnceCallback<void(content::CertificateRequestResultType)> callback)
      override;
  base::OnceClosure SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      net::ClientCertIdentityList client_certs,
      std::unique_ptr<content::ClientCertificateDelegate> delegate) override;
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
                       const std::string& raw_features,
                       const scoped_refptr<network::ResourceRequestBody>& body,
                       bool user_gesture,
                       bool opener_suppressed,
                       bool* no_javascript_access) override;
#if BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
  std::unique_ptr<content::VideoOverlayWindow>
  CreateWindowForVideoPictureInPicture(
      content::VideoPictureInPictureWindowController* controller) override;
  std::unique_ptr<content::DocumentOverlayWindow>
  CreateWindowForDocumentPictureInPicture(
      content::DocumentPictureInPictureWindowController* controller) override;
#endif
  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* additional_schemes) override;
  void GetAdditionalWebUISchemes(
      std::vector<std::string>* additional_schemes) override;
  void SiteInstanceDeleting(content::SiteInstance* site_instance) override;
  std::unique_ptr<net::ClientCertStore> CreateClientCertStore(
      content::BrowserContext* browser_context) override;
  std::unique_ptr<device::LocationProvider> OverrideSystemLocationProvider()
      override;
  void ConfigureNetworkContextParams(
      content::BrowserContext* browser_context,
      bool in_memory,
      const base::FilePath& relative_partition_path,
      network::mojom::NetworkContextParams* network_context_params,
      cert_verifier::mojom::CertVerifierCreationParams*
          cert_verifier_creation_params) override;
  network::mojom::NetworkContext* GetSystemNetworkContext() override;
  content::MediaObserver* GetMediaObserver() override;
  std::unique_ptr<content::DevToolsManagerDelegate>
  CreateDevToolsManagerDelegate() override;
  std::unique_ptr<content::BrowserMainParts> CreateBrowserMainParts(
      content::MainFunctionParams params) override;
  base::FilePath GetDefaultDownloadDirectory() override;
  scoped_refptr<network::SharedURLLoaderFactory>
  GetSystemSharedURLLoaderFactory() override;
  void OnNetworkServiceCreated(
      network::mojom::NetworkService* network_service) override;
  std::vector<base::FilePath> GetNetworkContextsParentDirectory() override;
  std::string GetProduct() override;
  void RegisterNonNetworkNavigationURLLoaderFactories(
      int frame_tree_node_id,
      ukm::SourceIdObj ukm_source_id,
      NonNetworkURLLoaderFactoryMap* factories) override;
  void RegisterNonNetworkWorkerMainResourceURLLoaderFactories(
      content::BrowserContext* browser_context,
      NonNetworkURLLoaderFactoryMap* factories) override;
  void RegisterNonNetworkSubresourceURLLoaderFactories(
      int render_process_id,
      int render_frame_id,
      const absl::optional<url::Origin>& request_initiator_origin,
      NonNetworkURLLoaderFactoryMap* factories) override;
  void RegisterNonNetworkServiceWorkerUpdateURLLoaderFactories(
      content::BrowserContext* browser_context,
      NonNetworkURLLoaderFactoryMap* factories) override;
  void CreateWebSocket(
      content::RenderFrameHost* frame,
      WebSocketFactory factory,
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const absl::optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client) override;
  bool WillInterceptWebSocket(content::RenderFrameHost*) override;
  bool WillCreateURLLoaderFactory(
      content::BrowserContext* browser_context,
      content::RenderFrameHost* frame,
      int render_process_id,
      URLLoaderFactoryType type,
      const url::Origin& request_initiator,
      absl::optional<int64_t> navigation_id,
      ukm::SourceIdObj ukm_source_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
      mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
          header_client,
      bool* bypass_redirect_checks,
      bool* disable_secure_dns,
      network::mojom::URLLoaderFactoryOverridePtr* factory_override) override;
  bool ShouldTreatURLSchemeAsFirstPartyWhenTopLevel(
      base::StringPiece scheme,
      bool is_embedded_origin_secure) override;
  void OverrideURLLoaderFactoryParams(
      content::BrowserContext* browser_context,
      const url::Origin& origin,
      bool is_for_isolated_world,
      network::mojom::URLLoaderFactoryParams* factory_params) override;
#if BUILDFLAG(IS_WIN)
  bool PreSpawnChild(sandbox::TargetPolicy* policy,
                     sandbox::mojom::Sandbox sandbox_type,
                     ChildSpawnFlags flags) override;
#endif
  void RegisterAssociatedInterfaceBindersForRenderFrameHost(
      content::RenderFrameHost& render_frame_host,
      blink::AssociatedInterfaceRegistry& associated_registry) override;

  bool HandleExternalProtocol(
      const GURL& url,
      content::WebContents::Getter web_contents_getter,
      int frame_tree_node_id,
      content::NavigationUIData* navigation_data,
      bool is_primary_main_frame,
      bool is_in_fenced_frame_tree,
      network::mojom::WebSandboxFlags sandbox_flags,
      ui::PageTransition page_transition,
      bool has_user_gesture,
      const absl::optional<url::Origin>& initiating_origin,
      content::RenderFrameHost* initiator_document,
      mojo::PendingRemote<network::mojom::URLLoaderFactory>* out_factory)
      override;
  std::unique_ptr<content::LoginDelegate> CreateLoginDelegate(
      const net::AuthChallengeInfo& auth_info,
      content::WebContents* web_contents,
      const content::GlobalRequestID& request_id,
      bool is_main_frame,
      const GURL& url,
      scoped_refptr<net::HttpResponseHeaders> response_headers,
      bool first_auth_attempt,
      LoginAuthRequiredCallback auth_required_callback) override;
  void SiteInstanceGotProcess(content::SiteInstance* site_instance) override;
  std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
  CreateURLLoaderThrottles(
      const network::ResourceRequest& request,
      content::BrowserContext* browser_context,
      const base::RepeatingCallback<content::WebContents*()>& wc_getter,
      content::NavigationUIData* navigation_ui_data,
      int frame_tree_node_id) override;
  base::flat_set<std::string> GetPluginMimeTypesWithExternalHandlers(
      content::BrowserContext* browser_context) override;
  bool IsSuitableHost(content::RenderProcessHost* process_host,
                      const GURL& site_url) override;
  bool ShouldUseProcessPerSite(content::BrowserContext* browser_context,
                               const GURL& effective_url) override;
  bool ArePersistentMediaDeviceIDsAllowed(
      content::BrowserContext* browser_context,
      const GURL& scope,
      const net::SiteForCookies& site_for_cookies,
      const absl::optional<url::Origin>& top_frame_origin) override;
  base::FilePath GetLoggingFileName(const base::CommandLine& cmd_line) override;

  // content::RenderProcessHostObserver:
  void RenderProcessHostDestroyed(content::RenderProcessHost* host) override;
  void RenderProcessReady(content::RenderProcessHost* host) override;
  void RenderProcessExited(
      content::RenderProcessHost* host,
      const content::ChildProcessTerminationInfo& info) override;

 private:
  content::SiteInstance* GetSiteInstanceFromAffinity(
      content::BrowserContext* browser_context,
      const GURL& url,
      content::RenderFrameHost* rfh) const;

  bool IsRendererSubFrame(int process_id) const;

  // pending_render_process => web contents.
  std::map<int, content::WebContents*> pending_processes_;

  std::set<int> renderer_is_subframe_;

  std::unique_ptr<PlatformNotificationService> notification_service_;
  std::unique_ptr<NotificationPresenter> notification_presenter_;

  Delegate* delegate_ = nullptr;

  std::string user_agent_override_ = "";

  // Simple shared ID generator, used by ProxyingURLLoaderFactory and
  // ProxyingWebSocket classes.
  uint64_t next_id_ = 0;

  std::unique_ptr<ElectronSerialDelegate> serial_delegate_;
  std::unique_ptr<ElectronBluetoothDelegate> bluetooth_delegate_;
  std::unique_ptr<ElectronHidDelegate> hid_delegate_;

#if BUILDFLAG(IS_MAC)
  ElectronBrowserMainParts* browser_main_parts_ = nullptr;
#endif
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_BROWSER_CLIENT_H_
