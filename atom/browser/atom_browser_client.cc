// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_client.h"

#if defined(OS_WIN)
#include <shlobj.h>
#endif

#include <memory>
#include <utility>

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/api/atom_api_protocol.h"
#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_navigation_throttle.h"
#include "atom/browser/atom_quota_permission_context.h"
#include "atom/browser/atom_resource_dispatcher_host_delegate.h"
#include "atom/browser/atom_speech_recognition_manager_delegate.h"
#include "atom/browser/child_web_contents_tracker.h"
#include "atom/browser/io_thread.h"
#include "atom/browser/native_window.h"
#include "atom/browser/session_preferences.h"
#include "atom/browser/web_contents_permission_helper.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/window_list.h"
#include "atom/common/google_api_key.h"
#include "atom/common/options_switches.h"
#include "atom/common/platform_util.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/no_destructor.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/printing/printing_message_filter.h"
#include "chrome/browser/speech/tts_message_filter.h"
#include "chrome/grit/generated_resources.h"
#include "components/net_log/chrome_net_log.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/web_preferences.h"
#include "electron/buildflags/buildflags.h"
#include "electron/grit/electron_resources.h"
#include "net/base/escape.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "ppapi/host/ppapi_host.h"
#include "services/device/public/cpp/geolocation/location_provider.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "v8/include/v8.h"

#if defined(USE_NSS_CERTS)
#include "net/ssl/client_cert_store_nss.h"
#elif defined(OS_WIN)
#include "net/ssl/client_cert_store_win.h"
#elif defined(OS_MACOSX)
#include "net/ssl/client_cert_store_mac.h"
#elif defined(USE_OPENSSL)
#include "net/ssl/client_cert_store.h"
#endif

#if BUILDFLAG(ENABLE_PEPPER_FLASH)
#include "chrome/browser/renderer_host/pepper/chrome_browser_pepper_host_factory.h"
#endif  // BUILDFLAG(ENABLE_PEPPER_FLASH)

#if BUILDFLAG(OVERRIDE_LOCATION_PROVIDER)
#include "atom/browser/fake_location_provider.h"
#endif  // BUILDFLAG(OVERRIDE_LOCATION_PROVIDER)

using content::BrowserThread;

namespace atom {

namespace {

// Next navigation should not restart renderer process.
bool g_suppress_renderer_process_restart = false;

// Custom schemes to be registered to handle service worker.
base::NoDestructor<std::string> g_custom_service_worker_schemes;

bool IsSameWebSite(content::BrowserContext* browser_context,
                   const GURL& src_url,
                   const GURL& dest_url) {
  return content::SiteInstance::IsSameWebSite(browser_context, src_url,
                                              dest_url) ||
         // `IsSameWebSite` doesn't seem to work for some URIs such as `file:`,
         // handle these scenarios by comparing only the site as defined by
         // `GetSiteForURL`.
         content::SiteInstance::GetSiteForURL(browser_context, dest_url) ==
             src_url;
}

}  // namespace

// static
void AtomBrowserClient::SuppressRendererProcessRestartForOnce() {
  g_suppress_renderer_process_restart = true;
}

void AtomBrowserClient::SetCustomServiceWorkerSchemes(
    const std::vector<std::string>& schemes) {
  *g_custom_service_worker_schemes = base::JoinString(schemes, ",");
}

AtomBrowserClient::AtomBrowserClient() {}

AtomBrowserClient::~AtomBrowserClient() {}

content::WebContents* AtomBrowserClient::GetWebContentsFromProcessID(
    int process_id) {
  // If the process is a pending process, we should use the web contents
  // for the frame host passed into OverrideSiteInstanceForNavigation.
  if (base::ContainsKey(pending_processes_, process_id))
    return pending_processes_[process_id];

  // Certain render process will be created with no associated render view,
  // for example: ServiceWorker.
  return WebContentsPreferences::GetWebContentsFromProcessID(process_id);
}

bool AtomBrowserClient::ShouldCreateNewSiteInstance(
    content::RenderFrameHost* render_frame_host,
    content::BrowserContext* browser_context,
    content::SiteInstance* current_instance,
    const GURL& url) {
  if (url.SchemeIs(url::kJavaScriptScheme))
    // "javacript:" scheme should always use same SiteInstance
    return false;

  int process_id = current_instance->GetProcess()->GetID();
  if (!IsRendererSandboxed(process_id)) {
    if (!RendererUsesNativeWindowOpen(process_id)) {
      // non-sandboxed renderers without native window.open should always create
      // a new SiteInstance
      return true;
    }
    auto* web_contents =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    if (!ChildWebContentsTracker::IsChildWebContents(web_contents)) {
      // Root WebContents should always create new process to make sure
      // native addons are loaded correctly after reload / navigation.
      // (Non-root WebContents opened by window.open() should try to
      //  reuse process to allow synchronous cross-window scripting.)
      return true;
    }
  }

  // Create new a SiteInstance if navigating to a different site.
  auto src_url = current_instance->GetSiteURL();
  return !IsSameWebSite(browser_context, src_url, url);
}

void AtomBrowserClient::AddProcessPreferences(
    int process_id,
    AtomBrowserClient::ProcessPreferences prefs) {
  process_preferences_[process_id] = prefs;
}

void AtomBrowserClient::RemoveProcessPreferences(int process_id) {
  process_preferences_.erase(process_id);
}

bool AtomBrowserClient::IsProcessObserved(int process_id) {
  return process_preferences_.find(process_id) != process_preferences_.end();
}

bool AtomBrowserClient::IsRendererSandboxed(int process_id) {
  auto it = process_preferences_.find(process_id);
  return it != process_preferences_.end() && it->second.sandbox;
}

bool AtomBrowserClient::RendererUsesNativeWindowOpen(int process_id) {
  auto it = process_preferences_.find(process_id);
  return it != process_preferences_.end() && it->second.native_window_open;
}

bool AtomBrowserClient::RendererDisablesPopups(int process_id) {
  auto it = process_preferences_.find(process_id);
  return it != process_preferences_.end() && it->second.disable_popups;
}

void AtomBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host,
    service_manager::mojom::ServiceRequest* service_request) {
  // When a render process is crashed, it might be reused.
  int process_id = host->GetID();
  if (IsProcessObserved(process_id))
    return;

  host->AddFilter(new printing::PrintingMessageFilter(process_id));
  host->AddFilter(new TtsMessageFilter(process_id, host->GetBrowserContext()));

  ProcessPreferences prefs;
  auto* web_preferences =
      WebContentsPreferences::From(GetWebContentsFromProcessID(process_id));
  if (web_preferences) {
    prefs.sandbox = web_preferences->IsEnabled(options::kSandbox);
    prefs.native_window_open =
        web_preferences->IsEnabled(options::kNativeWindowOpen);
    prefs.disable_popups = web_preferences->IsEnabled("disablePopups");
  }
  AddProcessPreferences(host->GetID(), prefs);
  // ensure the ProcessPreferences is removed later
  host->AddObserver(this);
}

content::SpeechRecognitionManagerDelegate*
AtomBrowserClient::CreateSpeechRecognitionManagerDelegate() {
  return new AtomSpeechRecognitionManagerDelegate;
}

void AtomBrowserClient::OverrideWebkitPrefs(content::RenderViewHost* host,
                                            content::WebPreferences* prefs) {
  prefs->javascript_enabled = true;
  prefs->web_security_enabled = true;
  prefs->plugins_enabled = true;
  prefs->dom_paste_enabled = true;
  prefs->allow_scripts_to_close_windows = true;
  prefs->javascript_can_access_clipboard = true;
  prefs->local_storage_enabled = true;
  prefs->databases_enabled = true;
  prefs->application_cache_enabled = true;
  prefs->allow_universal_access_from_file_urls = true;
  prefs->allow_file_access_from_file_urls = true;
  prefs->webgl1_enabled = true;
  prefs->webgl2_enabled = true;
  prefs->allow_running_insecure_content = false;
  prefs->default_minimum_page_scale_factor = 1.f;
  prefs->default_maximum_page_scale_factor = 1.f;
  prefs->navigate_on_drag_drop = false;

  // Custom preferences of guest page.
  auto* web_contents = content::WebContents::FromRenderViewHost(host);
  auto* web_preferences = WebContentsPreferences::From(web_contents);
  if (web_preferences)
    web_preferences->OverrideWebkitPrefs(prefs);
}

void AtomBrowserClient::OverrideSiteInstanceForNavigation(
    content::RenderFrameHost* rfh,
    content::BrowserContext* browser_context,
    const GURL& url,
    bool has_request_started,
    content::SiteInstance* candidate_instance,
    content::SiteInstance** new_instance) {
  if (g_suppress_renderer_process_restart) {
    g_suppress_renderer_process_restart = false;
    return;
  }

  content::SiteInstance* current_instance = rfh->GetSiteInstance();
  if (!ShouldCreateNewSiteInstance(rfh, browser_context, current_instance, url))
    return;

  // Do we have an affinity site to manage ?
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  auto* web_preferences = WebContentsPreferences::From(web_contents);
  std::string affinity;
  if (web_preferences &&
      web_preferences->GetPreference("affinity", &affinity) &&
      !affinity.empty()) {
    affinity = base::ToLowerASCII(affinity);
    auto iter = site_per_affinities.find(affinity);
    GURL dest_site = content::SiteInstance::GetSiteForURL(browser_context, url);
    if (iter != site_per_affinities.end() &&
        IsSameWebSite(browser_context, iter->second->GetSiteURL(), dest_site)) {
      *new_instance = iter->second;
    } else {
      site_per_affinities[affinity] = candidate_instance;
      *new_instance = candidate_instance;
      // Remember the original web contents for the pending renderer process.
      auto* pending_process = candidate_instance->GetProcess();
      pending_processes_[pending_process->GetID()] = web_contents;
    }
  } else {
    // OverrideSiteInstanceForNavigation will be called more than once during a
    // navigation (currently twice, on request and when it's about to commit in
    // the renderer), look at RenderFrameHostManager::GetFrameHostForNavigation.
    // In the default mode we should resuse the same site instance until the
    // request commits otherwise it will get destroyed. Currently there is no
    // unique lifetime tracker for a navigation request during site instance
    // creation. We check for the state of the request, which should be one of
    // (WAITING_FOR_RENDERER_RESPONSE, STARTED, RESPONSE_STARTED, FAILED) along
    // with the availability of a speculative render frame host.
    if (has_request_started) {
      *new_instance = current_instance;
      return;
    }

    *new_instance = candidate_instance;
    // Remember the original web contents for the pending renderer process.
    auto* pending_process = candidate_instance->GetProcess();
    pending_processes_[pending_process->GetID()] = web_contents;
  }
}

void AtomBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int process_id) {
  // Make sure we're about to launch a known executable
  {
    base::FilePath child_path;
    base::PathService::Get(content::CHILD_PROCESS_EXE, &child_path);

    base::ThreadRestrictions::ScopedAllowIO allow_io;
    CHECK(base::MakeAbsoluteFilePath(command_line->GetProgram()) == child_path);
  }

  std::string process_type =
      command_line->GetSwitchValueASCII(::switches::kProcessType);
  if (process_type != ::switches::kRendererProcess)
    return;

  // Copy following switches to child process.
  static const char* const kCommonSwitchNames[] = {switches::kStandardSchemes,
                                                   switches::kEnableSandbox,
                                                   switches::kSecureSchemes};
  command_line->CopySwitchesFrom(*base::CommandLine::ForCurrentProcess(),
                                 kCommonSwitchNames,
                                 arraysize(kCommonSwitchNames));

  // The registered service worker schemes.
  if (!g_custom_service_worker_schemes->empty())
    command_line->AppendSwitchASCII(switches::kRegisterServiceWorkerSchemes,
                                    *g_custom_service_worker_schemes);

#if defined(OS_WIN)
  // Append --app-user-model-id.
  PWSTR current_app_id;
  if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&current_app_id))) {
    command_line->AppendSwitchNative(switches::kAppUserModelId, current_app_id);
    CoTaskMemFree(current_app_id);
  }
#endif

  if (delegate_) {
    auto app_path = static_cast<api::App*>(delegate_)->GetAppPath();
    command_line->AppendSwitchPath(switches::kAppPath, app_path);
  }

  content::WebContents* web_contents = GetWebContentsFromProcessID(process_id);
  if (web_contents) {
    auto* web_preferences = WebContentsPreferences::From(web_contents);
    if (web_preferences)
      web_preferences->AppendCommandLineSwitches(command_line);
    SessionPreferences::AppendExtraCommandLineSwitches(
        web_contents->GetBrowserContext(), command_line);
  }
}

void AtomBrowserClient::DidCreatePpapiPlugin(content::BrowserPpapiHost* host) {
#if BUILDFLAG(ENABLE_PEPPER_FLASH)
  host->GetPpapiHost()->AddHostFactoryFilter(
      base::WrapUnique(new ChromeBrowserPepperHostFactory(host)));
#endif
}

std::string AtomBrowserClient::GetGeolocationApiKey() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string api_key;
  if (!env->GetVar("GOOGLE_API_KEY", &api_key))
    api_key = GOOGLEAPIS_API_KEY;
  return api_key;
}

content::QuotaPermissionContext*
AtomBrowserClient::CreateQuotaPermissionContext() {
  return new AtomQuotaPermissionContext;
}

void AtomBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    content::ResourceType resource_type,
    bool strict_enforcement,
    bool expired_previous_decision,
    const base::Callback<void(content::CertificateRequestResultType)>&
        callback) {
  if (delegate_) {
    delegate_->AllowCertificateError(
        web_contents, cert_error, ssl_info, request_url, resource_type,
        strict_enforcement, expired_previous_decision, callback);
  }
}

void AtomBrowserClient::SelectClientCertificate(
    content::WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    net::ClientCertIdentityList client_certs,
    std::unique_ptr<content::ClientCertificateDelegate> delegate) {
  if (!client_certs.empty() && delegate_) {
    delegate_->SelectClientCertificate(web_contents, cert_request_info,
                                       std::move(client_certs),
                                       std::move(delegate));
  }
}

void AtomBrowserClient::ResourceDispatcherHostCreated() {
  resource_dispatcher_host_delegate_.reset(
      new AtomResourceDispatcherHostDelegate);
  content::ResourceDispatcherHost::Get()->SetDelegate(
      resource_dispatcher_host_delegate_.get());
}

bool AtomBrowserClient::CanCreateWindow(
    content::RenderFrameHost* opener,
    const GURL& opener_url,
    const GURL& opener_top_level_frame_url,
    const GURL& source_origin,
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
    bool* no_javascript_access) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  int opener_render_process_id = opener->GetProcess()->GetID();

  if (IsRendererSandboxed(opener_render_process_id)) {
    *no_javascript_access = false;
    return true;
  }

  if (RendererUsesNativeWindowOpen(opener_render_process_id)) {
    if (RendererDisablesPopups(opener_render_process_id)) {
      // <webview> without allowpopups attribute should return
      // null from window.open calls
      return false;
    } else {
      *no_javascript_access = false;
      return true;
    }
  }

  if (delegate_) {
    return delegate_->CanCreateWindow(
        opener, opener_url, opener_top_level_frame_url, source_origin,
        container_type, target_url, referrer, frame_name, disposition, features,
        additional_features, body, user_gesture, opener_suppressed,
        no_javascript_access);
  }

  return false;
}

void AtomBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
    std::vector<std::string>* additional_schemes) {
  auto schemes_list = api::GetStandardSchemes();
  if (!schemes_list.empty())
    additional_schemes->insert(additional_schemes->end(), schemes_list.begin(),
                               schemes_list.end());
  additional_schemes->push_back(content::kChromeDevToolsScheme);
}

void AtomBrowserClient::SiteInstanceDeleting(
    content::SiteInstance* site_instance) {
  // We are storing weak_ptr, is it fundamental to maintain the map up-to-date
  // when an instance is destroyed.
  for (auto iter = site_per_affinities.begin();
       iter != site_per_affinities.end(); ++iter) {
    if (iter->second == site_instance) {
      site_per_affinities.erase(iter);
      break;
    }
  }
}

std::unique_ptr<net::ClientCertStore> AtomBrowserClient::CreateClientCertStore(
    content::ResourceContext* resource_context) {
#if defined(USE_NSS_CERTS)
  return std::make_unique<net::ClientCertStoreNSS>(
      net::ClientCertStoreNSS::PasswordDelegateFactory());
#elif defined(OS_WIN)
  return std::make_unique<net::ClientCertStoreWin>();
#elif defined(OS_MACOSX)
  return std::make_unique<net::ClientCertStoreMac>();
#elif defined(USE_OPENSSL)
  return std::unique_ptr<net::ClientCertStore>();
#endif
}

std::unique_ptr<device::LocationProvider>
AtomBrowserClient::OverrideSystemLocationProvider() {
#if BUILDFLAG(OVERRIDE_LOCATION_PROVIDER)
  return std::make_unique<FakeLocationProvider>();
#else
  return nullptr;
#endif
}

network::mojom::NetworkContextPtr AtomBrowserClient::CreateNetworkContext(
    content::BrowserContext* browser_context,
    bool /*in_memory*/,
    const base::FilePath& /*relative_partition_path*/) {
  if (!browser_context)
    return nullptr;
  return static_cast<AtomBrowserContext*>(browser_context)->GetNetworkContext();
}

void AtomBrowserClient::RegisterOutOfProcessServices(
    OutOfProcessServiceMap* services) {
  (*services)[proxy_resolver::mojom::kProxyResolverServiceName] =
      base::BindRepeating(&l10n_util::GetStringUTF16,
                          IDS_UTILITY_PROCESS_PROXY_RESOLVER_NAME);
}

std::unique_ptr<base::Value> AtomBrowserClient::GetServiceManifestOverlay(
    base::StringPiece name) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  int id = -1;
  if (name == content::mojom::kBrowserServiceName)
    id = IDR_ELECTRON_CONTENT_BROWSER_MANIFEST_OVERLAY;
  else if (name == content::mojom::kPackagedServicesServiceName)
    id = IDR_ELECTRON_CONTENT_PACKAGED_SERVICES_MANIFEST_OVERLAY;

  if (id == -1)
    return nullptr;

  base::StringPiece manifest_contents = rb.GetRawDataResource(id);
  return base::JSONReader::Read(manifest_contents);
}

net::NetLog* AtomBrowserClient::GetNetLog() {
  return AtomBrowserMainParts::Get()->net_log();
}

brightray::BrowserMainParts* AtomBrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams& params) {
  return new AtomBrowserMainParts(params);
}

void AtomBrowserClient::WebNotificationAllowed(
    int render_process_id,
    const base::Callback<void(bool, bool)>& callback) {
  content::WebContents* web_contents =
      WebContentsPreferences::GetWebContentsFromProcessID(render_process_id);
  if (!web_contents) {
    callback.Run(false, false);
    return;
  }
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  if (!permission_helper) {
    callback.Run(false, false);
    return;
  }
  permission_helper->RequestWebNotificationPermission(
      base::Bind(callback, web_contents->IsAudioMuted()));
}

void AtomBrowserClient::RenderProcessHostDestroyed(
    content::RenderProcessHost* host) {
  int process_id = host->GetID();
  pending_processes_.erase(process_id);
  RemoveProcessPreferences(process_id);
}

void AtomBrowserClient::RenderProcessReady(content::RenderProcessHost* host) {
  render_process_host_pids_[host->GetID()] =
      base::GetProcId(host->GetProcess().Handle());
  if (delegate_) {
    static_cast<api::App*>(delegate_)->RenderProcessReady(host);
  }
}

void AtomBrowserClient::RenderProcessExited(
    content::RenderProcessHost* host,
    const content::ChildProcessTerminationInfo& info) {
  auto host_pid = render_process_host_pids_.find(host->GetID());
  if (host_pid != render_process_host_pids_.end()) {
    if (delegate_) {
      static_cast<api::App*>(delegate_)->RenderProcessDisconnected(
          host_pid->second);
    }
    render_process_host_pids_.erase(host_pid);
  }
}

void OnOpenExternal(const GURL& escaped_url, bool allowed) {
  if (allowed)
    platform_util::OpenExternal(
#if defined(OS_WIN)
        base::UTF8ToUTF16(escaped_url.spec()),
#else
        escaped_url,
#endif
        true);
}

void HandleExternalProtocolInUI(
    const GURL& url,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    bool has_user_gesture) {
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return;

  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  if (!permission_helper)
    return;

  GURL escaped_url(net::EscapeExternalHandlerValue(url.spec()));
  auto callback = base::Bind(&OnOpenExternal, escaped_url);
  permission_helper->RequestOpenExternalPermission(callback, has_user_gesture,
                                                   url);
}

bool AtomBrowserClient::HandleExternalProtocol(
    const GURL& url,
    content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
    int child_id,
    content::NavigationUIData* navigation_data,
    bool is_main_frame,
    ui::PageTransition page_transition,
    bool has_user_gesture) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&HandleExternalProtocolInUI, url, web_contents_getter,
                     has_user_gesture));
  return true;
}

std::vector<std::unique_ptr<content::NavigationThrottle>>
AtomBrowserClient::CreateThrottlesForNavigation(
    content::NavigationHandle* handle) {
  std::vector<std::unique_ptr<content::NavigationThrottle>> throttles;
  throttles.push_back(std::make_unique<AtomNavigationThrottle>(handle));
  return throttles;
}

}  // namespace atom
