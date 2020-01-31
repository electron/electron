// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_browser_client.h"

#if defined(OS_WIN)
#include <shlobj.h>
#endif

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/lazy_instance.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_version.h"
#include "components/net_log/chrome_net_log.h"
#include "components/network_hints/common/network_hints.mojom.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/overlay_window.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/web_preferences.h"
#include "electron/buildflags/buildflags.h"
#include "electron/grit/electron_resources.h"
#include "extensions/browser/extension_navigation_ui_data.h"
#include "extensions/browser/extension_protocols.h"
#include "extensions/common/constants.h"
#include "net/base/escape.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "ppapi/host/ppapi_host.h"
#include "printing/buildflags/buildflags.h"
#include "services/device/public/cpp/geolocation/location_provider.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/service_manager/public/cpp/binder_map.h"
#include "shell/app/manifests.h"
#include "shell/browser/api/atom_api_app.h"
#include "shell/browser/api/atom_api_protocol.h"
#include "shell/browser/api/atom_api_session.h"
#include "shell/browser/api/atom_api_web_contents.h"
#include "shell/browser/api/atom_api_web_request.h"
#include "shell/browser/atom_autofill_driver_factory.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/browser/atom_browser_main_parts.h"
#include "shell/browser/atom_navigation_throttle.h"
#include "shell/browser/atom_paths.h"
#include "shell/browser/atom_quota_permission_context.h"
#include "shell/browser/atom_speech_recognition_manager_delegate.h"
#include "shell/browser/child_web_contents_tracker.h"
#include "shell/browser/font_defaults.h"
#include "shell/browser/media/media_capture_devices_dispatcher.h"
#include "shell/browser/native_window.h"
#include "shell/browser/net/network_context_service.h"
#include "shell/browser/net/network_context_service_factory.h"
#include "shell/browser/net/proxying_url_loader_factory.h"
#include "shell/browser/net/system_network_context_manager.h"
#include "shell/browser/network_hints_handler_impl.h"
#include "shell/browser/notifications/notification_presenter.h"
#include "shell/browser/notifications/platform_notification_service.h"
#include "shell/browser/session_preferences.h"
#include "shell/browser/ui/devtools_manager_delegate.h"
#include "shell/browser/web_contents_permission_helper.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/browser/window_list.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/application_info.h"
#include "shell/common/options_switches.h"
#include "shell/common/platform_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/native_theme/native_theme.h"
#include "v8/include/v8.h"

#if defined(OS_WIN)
#include "sandbox/win/src/sandbox_policy.h"
#endif

#if defined(USE_NSS_CERTS)
#include "net/ssl/client_cert_store_nss.h"
#elif defined(OS_WIN)
#include "net/ssl/client_cert_store_win.h"
#elif defined(OS_MACOSX)
#include "net/ssl/client_cert_store_mac.h"
#elif defined(USE_OPENSSL)
#include "net/ssl/client_cert_store.h"
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "chrome/browser/spellchecker/spell_check_host_chrome_impl.h"  // nogncheck
#include "components/spellcheck/common/spellcheck.mojom.h"  // nogncheck
#endif

#if BUILDFLAG(ENABLE_PEPPER_FLASH)
#include "chrome/browser/renderer_host/pepper/chrome_browser_pepper_host_factory.h"
#endif  // BUILDFLAG(ENABLE_PEPPER_FLASH)

#if BUILDFLAG(OVERRIDE_LOCATION_PROVIDER)
#include "shell/browser/fake_location_provider.h"
#endif  // BUILDFLAG(OVERRIDE_LOCATION_PROVIDER)

#if BUILDFLAG(ENABLE_TTS)
#include "chrome/browser/speech/tts_controller_delegate_impl.h"
#endif  // BUILDFLAG(ENABLE_TTS)

#if BUILDFLAG(ENABLE_PRINTING)
#include "chrome/browser/printing/printing_message_filter.h"
#endif  // BUILDFLAG(ENABLE_PRINTING)

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/browser/extension_message_filter.h"
#include "extensions/browser/extension_navigation_throttle.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/process_map.h"
#include "extensions/common/extension.h"
#include "shell/browser/extensions/atom_extension_system.h"
#endif

#if defined(OS_MACOSX)
#include "content/common/mac_helpers.h"
#include "content/public/common/child_process_host.h"
#endif

using content::BrowserThread;

namespace electron {

namespace {

// Next navigation should not restart renderer process.
bool g_suppress_renderer_process_restart = false;

bool IsSameWebSite(content::BrowserContext* browser_context,
                   content::SiteInstance* site_instance,
                   const GURL& dest_url) {
  return site_instance->IsSameSiteWithURL(dest_url) ||
         // `IsSameSiteWithURL` doesn't seem to work for some URIs such as
         // `file:`, handle these scenarios by comparing only the site as
         // defined by `GetSiteForURL`.
         (content::SiteInstance::GetSiteForURL(browser_context, dest_url) ==
          site_instance->GetSiteURL());
}

AtomBrowserClient* g_browser_client = nullptr;

base::LazyInstance<std::string>::DestructorAtExit
    g_io_thread_application_locale = LAZY_INSTANCE_INITIALIZER;

base::NoDestructor<std::string> g_application_locale;

void SetApplicationLocaleOnIOThread(const std::string& locale) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  g_io_thread_application_locale.Get() = locale;
}

void BindNetworkHintsHandler(
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<network_hints::mojom::NetworkHintsHandler> receiver) {
  NetworkHintsHandlerImpl::Create(frame_host, std::move(receiver));
}

#if defined(OS_WIN)
const base::FilePath::StringPieceType kPathDelimiter = FILE_PATH_LITERAL(";");
#else
const base::FilePath::StringPieceType kPathDelimiter = FILE_PATH_LITERAL(":");
#endif

}  // namespace

// static
void AtomBrowserClient::SuppressRendererProcessRestartForOnce() {
  g_suppress_renderer_process_restart = true;
}

AtomBrowserClient* AtomBrowserClient::Get() {
  return g_browser_client;
}

// static
void AtomBrowserClient::SetApplicationLocale(const std::string& locale) {
  if (!BrowserThread::IsThreadInitialized(BrowserThread::IO) ||
      !base::PostTask(
          FROM_HERE, {BrowserThread::IO},
          base::BindOnce(&SetApplicationLocaleOnIOThread, locale))) {
    g_io_thread_application_locale.Get() = locale;
  }
  *g_application_locale = locale;
}

AtomBrowserClient::AtomBrowserClient() {
  DCHECK(!g_browser_client);
  g_browser_client = this;
}

AtomBrowserClient::~AtomBrowserClient() {
  DCHECK(g_browser_client);
  g_browser_client = nullptr;
}

content::WebContents* AtomBrowserClient::GetWebContentsFromProcessID(
    int process_id) {
  // If the process is a pending process, we should use the web contents
  // for the frame host passed into RegisterPendingProcess.
  const auto iter = pending_processes_.find(process_id);
  if (iter != std::end(pending_processes_))
    return iter->second;

  // Certain render process will be created with no associated render view,
  // for example: ServiceWorker.
  return WebContentsPreferences::GetWebContentsFromProcessID(process_id);
}

bool AtomBrowserClient::ShouldForceNewSiteInstance(
    content::RenderFrameHost* current_rfh,
    content::RenderFrameHost* speculative_rfh,
    content::BrowserContext* browser_context,
    const GURL& url,
    bool has_response_started) const {
  if (url.SchemeIs(url::kJavaScriptScheme))
    // "javacript:" scheme should always use same SiteInstance
    return false;
  if (url.SchemeIs(extensions::kExtensionScheme))
    return false;

  content::SiteInstance* current_instance = current_rfh->GetSiteInstance();
  content::SiteInstance* speculative_instance =
      speculative_rfh ? speculative_rfh->GetSiteInstance() : nullptr;
  int process_id = current_instance->GetProcess()->GetID();
  if (NavigationWasRedirectedCrossSite(browser_context, current_instance,
                                       speculative_instance, url,
                                       has_response_started)) {
    // Navigation was redirected. We can't force the current, speculative or a
    // new unrelated site instance to be used. Delegate to the content layer.
    return false;
  } else if (IsRendererSandboxed(process_id)) {
    // Renderer is sandboxed, delegate the decision to the content layer for all
    // origins.
    return false;
  } else if (!RendererUsesNativeWindowOpen(process_id)) {
    // non-sandboxed renderers without native window.open should always create
    // a new SiteInstance
    return true;
  } else {
    auto* web_contents = content::WebContents::FromRenderFrameHost(current_rfh);
    if (!ChildWebContentsTracker::FromWebContents(web_contents)) {
      // Root WebContents should always create new process to make sure
      // native addons are loaded correctly after reload / navigation.
      // (Non-root WebContents opened by window.open() should try to
      //  reuse process to allow synchronous cross-window scripting.)
      return true;
    }
  }

  // Create new a SiteInstance if navigating to a different site.
  return !IsSameWebSite(browser_context, current_instance, url);
}

bool AtomBrowserClient::NavigationWasRedirectedCrossSite(
    content::BrowserContext* browser_context,
    content::SiteInstance* current_instance,
    content::SiteInstance* speculative_instance,
    const GURL& dest_url,
    bool has_response_started) const {
  bool navigation_was_redirected = false;
  if (has_response_started) {
    navigation_was_redirected =
        !IsSameWebSite(browser_context, current_instance, dest_url);
  } else {
    navigation_was_redirected =
        speculative_instance &&
        !IsSameWebSite(browser_context, speculative_instance, dest_url);
  }

  return navigation_was_redirected;
}

void AtomBrowserClient::AddProcessPreferences(
    int process_id,
    AtomBrowserClient::ProcessPreferences prefs) {
  process_preferences_[process_id] = prefs;
}

void AtomBrowserClient::RemoveProcessPreferences(int process_id) {
  process_preferences_.erase(process_id);
}

bool AtomBrowserClient::IsProcessObserved(int process_id) const {
  return process_preferences_.find(process_id) != process_preferences_.end();
}

bool AtomBrowserClient::IsRendererSandboxed(int process_id) const {
  auto it = process_preferences_.find(process_id);
  return it != process_preferences_.end() && it->second.sandbox;
}

bool AtomBrowserClient::RendererUsesNativeWindowOpen(int process_id) const {
  auto it = process_preferences_.find(process_id);
  return it != process_preferences_.end() && it->second.native_window_open;
}

bool AtomBrowserClient::RendererDisablesPopups(int process_id) const {
  auto it = process_preferences_.find(process_id);
  return it != process_preferences_.end() && it->second.disable_popups;
}

std::string AtomBrowserClient::GetAffinityPreference(
    content::RenderFrameHost* rfh) const {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  auto* web_preferences = WebContentsPreferences::From(web_contents);
  std::string affinity;
  if (web_preferences &&
      web_preferences->GetPreference("affinity", &affinity) &&
      !affinity.empty()) {
    affinity = base::ToLowerASCII(affinity);
  }

  return affinity;
}

content::SiteInstance* AtomBrowserClient::GetSiteInstanceFromAffinity(
    content::BrowserContext* browser_context,
    const GURL& url,
    content::RenderFrameHost* rfh) const {
  std::string affinity = GetAffinityPreference(rfh);
  if (!affinity.empty()) {
    auto iter = site_per_affinities_.find(affinity);
    GURL dest_site = content::SiteInstance::GetSiteForURL(browser_context, url);
    if (iter != site_per_affinities_.end() &&
        IsSameWebSite(browser_context, iter->second, dest_site)) {
      return iter->second;
    }
  }

  return nullptr;
}

void AtomBrowserClient::ConsiderSiteInstanceForAffinity(
    content::RenderFrameHost* rfh,
    content::SiteInstance* site_instance) {
  std::string affinity = GetAffinityPreference(rfh);
  if (!affinity.empty()) {
    site_per_affinities_[affinity] = site_instance;
  }
}

bool AtomBrowserClient::IsRendererSubFrame(int process_id) const {
  return base::Contains(renderer_is_subframe_, process_id);
}

void AtomBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  // When a render process is crashed, it might be reused.
  int process_id = host->GetID();
  if (IsProcessObserved(process_id))
    return;

  auto* browser_context = host->GetBrowserContext();

#if BUILDFLAG(ENABLE_PRINTING)
  host->AddFilter(
      new printing::PrintingMessageFilter(process_id, browser_context));
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  host->AddFilter(
      new extensions::ExtensionMessageFilter(process_id, browser_context));
#endif

  ProcessPreferences prefs;
  auto* web_preferences =
      WebContentsPreferences::From(GetWebContentsFromProcessID(process_id));
  if (web_preferences) {
    prefs.sandbox = web_preferences->IsEnabled(options::kSandbox);
    prefs.native_window_open =
        web_preferences->IsEnabled(options::kNativeWindowOpen);
    prefs.disable_popups = web_preferences->IsEnabled("disablePopups");
    prefs.web_security = web_preferences->IsEnabled(options::kWebSecurity,
                                                    true /* default value */);
    prefs.browser_context = host->GetBrowserContext();
  }

  AddProcessPreferences(host->GetID(), prefs);
  // ensure the ProcessPreferences is removed later
  host->AddObserver(this);
}

content::SpeechRecognitionManagerDelegate*
AtomBrowserClient::CreateSpeechRecognitionManagerDelegate() {
  return new AtomSpeechRecognitionManagerDelegate;
}

content::TtsControllerDelegate* AtomBrowserClient::GetTtsControllerDelegate() {
#if BUILDFLAG(ENABLE_TTS)
  return TtsControllerDelegateImpl::GetInstance();
#else
  return nullptr;
#endif
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
#if !BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
  prefs->picture_in_picture_enabled = false;
#endif

  SetFontDefaults(prefs);

  // Custom preferences of guest page.
  auto* web_contents = content::WebContents::FromRenderViewHost(host);
  auto* web_preferences = WebContentsPreferences::From(web_contents);
  if (web_preferences)
    web_preferences->OverrideWebkitPrefs(prefs);
}

void AtomBrowserClient::SetCanUseCustomSiteInstance(bool should_disable) {
  disable_process_restart_tricks_ = should_disable;
}

bool AtomBrowserClient::CanUseCustomSiteInstance() {
  return disable_process_restart_tricks_;
}

content::ContentBrowserClient::SiteInstanceForNavigationType
AtomBrowserClient::ShouldOverrideSiteInstanceForNavigation(
    content::RenderFrameHost* current_rfh,
    content::RenderFrameHost* speculative_rfh,
    content::BrowserContext* browser_context,
    const GURL& url,
    bool has_navigation_started,
    bool has_response_started,
    content::SiteInstance** affinity_site_instance) const {
  if (g_suppress_renderer_process_restart) {
    g_suppress_renderer_process_restart = false;
    return SiteInstanceForNavigationType::ASK_CHROMIUM;
  }

  // Do we have an affinity site to manage ?
  content::SiteInstance* site_instance_from_affinity =
      GetSiteInstanceFromAffinity(browser_context, url, current_rfh);
  if (site_instance_from_affinity) {
    *affinity_site_instance = site_instance_from_affinity;
    return SiteInstanceForNavigationType::FORCE_AFFINITY;
  }

  if (!ShouldForceNewSiteInstance(current_rfh, speculative_rfh, browser_context,
                                  url, has_response_started)) {
    return SiteInstanceForNavigationType::ASK_CHROMIUM;
  }

  // ShouldOverrideSiteInstanceForNavigation will be called more than once
  // during a navigation (currently twice, on request and when it's about
  // to commit in the renderer), look at
  // RenderFrameHostManager::GetFrameHostForNavigation.
  // In the default mode we should reuse the same site instance until the
  // request commits otherwise it will get destroyed. Currently there is no
  // unique lifetime tracker for a navigation request during site instance
  // creation. We check for the state of the request, which should be one of
  // (WAITING_FOR_RENDERER_RESPONSE, STARTED, RESPONSE_STARTED, FAILED) along
  // with the availability of a speculative render frame host.
  if (has_response_started) {
    return SiteInstanceForNavigationType::FORCE_CURRENT;
  }

  if (!has_navigation_started) {
    // If the navigation didn't start yet, ignore any candidate site instance.
    // If such instance exists, it belongs to a previous navigation still
    // taking place. Fixes https://github.com/electron/electron/issues/17576.
    return SiteInstanceForNavigationType::FORCE_NEW;
  }

  return SiteInstanceForNavigationType::FORCE_CANDIDATE_OR_NEW;
}

void AtomBrowserClient::RegisterPendingSiteInstance(
    content::RenderFrameHost* rfh,
    content::SiteInstance* pending_site_instance) {
  // Do we have an affinity site to manage?
  ConsiderSiteInstanceForAffinity(rfh, pending_site_instance);

  // Remember the original web contents for the pending renderer process.
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  auto* pending_process = pending_site_instance->GetProcess();
  pending_processes_[pending_process->GetID()] = web_contents;

  if (rfh->GetParent())
    renderer_is_subframe_.insert(pending_process->GetID());
  else
    renderer_is_subframe_.erase(pending_process->GetID());
}

void AtomBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int process_id) {
  // Make sure we're about to launch a known executable
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    base::FilePath child_path;
    base::FilePath program =
        base::MakeAbsoluteFilePath(command_line->GetProgram());
#if defined(OS_MACOSX)
    auto renderer_child_path = content::ChildProcessHost::GetChildPath(
        content::ChildProcessHost::CHILD_RENDERER);
    auto gpu_child_path = content::ChildProcessHost::GetChildPath(
        content::ChildProcessHost::CHILD_GPU);
    auto plugin_child_path = content::ChildProcessHost::GetChildPath(
        content::ChildProcessHost::CHILD_PLUGIN);
    if (program != renderer_child_path && program != gpu_child_path &&
        program != plugin_child_path) {
      child_path = content::ChildProcessHost::GetChildPath(
          content::ChildProcessHost::CHILD_NORMAL);
      CHECK_EQ(program, child_path)
          << "Aborted from launching unexpected helper executable";
    }
#else
    base::PathService::Get(content::CHILD_PROCESS_EXE, &child_path);
    CHECK_EQ(program, child_path);
#endif
  }

  std::string process_type =
      command_line->GetSwitchValueASCII(::switches::kProcessType);

  if (process_type == ::switches::kUtilityProcess ||
      process_type == ::switches::kRendererProcess) {
    // Copy following switches to child process.
    static const char* const kCommonSwitchNames[] = {
        switches::kStandardSchemes,      switches::kEnableSandbox,
        switches::kSecureSchemes,        switches::kBypassCSPSchemes,
        switches::kCORSSchemes,          switches::kFetchSchemes,
        switches::kServiceWorkerSchemes, switches::kEnableApiFilteringLogging};
    command_line->CopySwitchesFrom(*base::CommandLine::ForCurrentProcess(),
                                   kCommonSwitchNames,
                                   base::size(kCommonSwitchNames));
  }

  if (process_type == ::switches::kRendererProcess) {
#if defined(OS_WIN)
    // Append --app-user-model-id.
    PWSTR current_app_id;
    if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&current_app_id))) {
      command_line->AppendSwitchNative(switches::kAppUserModelId,
                                       current_app_id);
      CoTaskMemFree(current_app_id);
    }
#endif

    if (delegate_) {
      auto app_path = static_cast<api::App*>(delegate_)->GetAppPath();
      command_line->AppendSwitchPath(switches::kAppPath, app_path);
    }

    content::WebContents* web_contents =
        GetWebContentsFromProcessID(process_id);
    if (web_contents) {
      auto* web_preferences = WebContentsPreferences::From(web_contents);
      if (web_preferences)
        web_preferences->AppendCommandLineSwitches(
            command_line, IsRendererSubFrame(process_id));
      auto preloads = SessionPreferences::GetValidPreloads(
          web_contents->GetBrowserContext());
      if (!preloads.empty())
        command_line->AppendSwitchNative(
            switches::kPreloadScripts,
            base::JoinString(preloads, kPathDelimiter));
      if (CanUseCustomSiteInstance()) {
        command_line->AppendSwitch(
            switches::kDisableElectronSiteInstanceOverrides);
      }
    }
  }
}

void AtomBrowserClient::DidCreatePpapiPlugin(content::BrowserPpapiHost* host) {
#if BUILDFLAG(ENABLE_PEPPER_FLASH)
  host->GetPpapiHost()->AddHostFactoryFilter(
      base::WrapUnique(new ChromeBrowserPepperHostFactory(host)));
#endif
}

// attempt to get api key from env
std::string AtomBrowserClient::GetGeolocationApiKey() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string api_key;
  env->GetVar("GOOGLE_API_KEY", &api_key);
  return api_key;
}

scoped_refptr<content::QuotaPermissionContext>
AtomBrowserClient::CreateQuotaPermissionContext() {
  return new AtomQuotaPermissionContext;
}

content::GeneratedCodeCacheSettings
AtomBrowserClient::GetGeneratedCodeCacheSettings(
    content::BrowserContext* context) {
  // TODO(deepak1556): Use platform cache directory.
  base::FilePath cache_path = context->GetPath();
  // If we pass 0 for size, disk_cache will pick a default size using the
  // heuristics based on available disk size. These are implemented in
  // disk_cache::PreferredCacheSize in net/disk_cache/cache_util.cc.
  return content::GeneratedCodeCacheSettings(true, 0, cache_path);
}

void AtomBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    bool is_main_frame_request,
    bool strict_enforcement,
    base::OnceCallback<void(content::CertificateRequestResultType)> callback) {
  if (delegate_) {
    delegate_->AllowCertificateError(web_contents, cert_error, ssl_info,
                                     request_url, is_main_frame_request,
                                     strict_enforcement, std::move(callback));
  }
}

base::OnceClosure AtomBrowserClient::SelectClientCertificate(
    content::WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    net::ClientCertIdentityList client_certs,
    std::unique_ptr<content::ClientCertificateDelegate> delegate) {
  if (!client_certs.empty() && delegate_) {
    delegate_->SelectClientCertificate(web_contents, cert_request_info,
                                       std::move(client_certs),
                                       std::move(delegate));
  }
  return base::OnceClosure();
}

bool AtomBrowserClient::CanCreateWindow(
    content::RenderFrameHost* opener,
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
    bool* no_javascript_access) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  int opener_render_process_id = opener->GetProcess()->GetID();

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

#if BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
std::unique_ptr<content::OverlayWindow>
AtomBrowserClient::CreateWindowForPictureInPicture(
    content::PictureInPictureWindowController* controller) {
  return content::OverlayWindow::Create(controller);
}
#endif

void AtomBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
    std::vector<std::string>* additional_schemes) {
  auto schemes_list = api::GetStandardSchemes();
  if (!schemes_list.empty())
    additional_schemes->insert(additional_schemes->end(), schemes_list.begin(),
                               schemes_list.end());
  additional_schemes->push_back(content::kChromeDevToolsScheme);
}

void AtomBrowserClient::GetAdditionalWebUISchemes(
    std::vector<std::string>* additional_schemes) {
  additional_schemes->push_back(content::kChromeDevToolsScheme);
}

void AtomBrowserClient::SiteInstanceGotProcess(
    content::SiteInstance* site_instance) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  auto* browser_context =
      static_cast<AtomBrowserContext*>(site_instance->GetBrowserContext());
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser_context);
  const extensions::Extension* extension =
      registry->enabled_extensions().GetExtensionOrAppByURL(
          site_instance->GetSiteURL());
  if (!extension)
    return;

  extensions::ProcessMap::Get(browser_context)
      ->Insert(extension->id(), site_instance->GetProcess()->GetID(),
               site_instance->GetId());

  base::PostTask(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&extensions::InfoMap::RegisterExtensionProcess,
                     browser_context->extension_system()->info_map(),
                     extension->id(), site_instance->GetProcess()->GetID(),
                     site_instance->GetId()));
#endif  // BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
}

void AtomBrowserClient::SiteInstanceDeleting(
    content::SiteInstance* site_instance) {
  // We are storing weak_ptr, is it fundamental to maintain the map up-to-date
  // when an instance is destroyed.
  for (auto iter = site_per_affinities_.begin();
       iter != site_per_affinities_.end(); ++iter) {
    if (iter->second == site_instance) {
      site_per_affinities_.erase(iter);
      break;
    }
  }

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // Don't do anything if we're shutting down.
  if (content::BrowserMainRunner::ExitedMainMessageLoop())
    return;

  auto* browser_context =
      static_cast<AtomBrowserContext*>(site_instance->GetBrowserContext());
  // If this isn't an extension renderer there's nothing to do.
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser_context);
  const extensions::Extension* extension =
      registry->enabled_extensions().GetExtensionOrAppByURL(
          site_instance->GetSiteURL());
  if (!extension)
    return;

  extensions::ProcessMap::Get(browser_context)
      ->Remove(extension->id(), site_instance->GetProcess()->GetID(),
               site_instance->GetId());

  base::PostTask(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&extensions::InfoMap::UnregisterExtensionProcess,
                     browser_context->extension_system()->info_map(),
                     extension->id(), site_instance->GetProcess()->GetID(),
                     site_instance->GetId()));
#endif
}

std::unique_ptr<net::ClientCertStore> AtomBrowserClient::CreateClientCertStore(
    content::BrowserContext* browser_context) {
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

mojo::Remote<network::mojom::NetworkContext>
AtomBrowserClient::CreateNetworkContext(
    content::BrowserContext* browser_context,
    bool /*in_memory*/,
    const base::FilePath& /*relative_partition_path*/) {
  DCHECK(browser_context);
  return NetworkContextServiceFactory::GetForContext(browser_context)
      ->CreateNetworkContext();
}

network::mojom::NetworkContext* AtomBrowserClient::GetSystemNetworkContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(g_browser_process->system_network_context_manager());
  return g_browser_process->system_network_context_manager()->GetContext();
}

base::Optional<service_manager::Manifest>
AtomBrowserClient::GetServiceManifestOverlay(base::StringPiece name) {
  if (name == content::mojom::kBrowserServiceName)
    return GetElectronContentBrowserOverlayManifest();
  return base::nullopt;
}

std::unique_ptr<content::BrowserMainParts>
AtomBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& params) {
  return std::make_unique<AtomBrowserMainParts>(params);
}

void AtomBrowserClient::WebNotificationAllowed(
    int render_process_id,
    base::OnceCallback<void(bool, bool)> callback) {
  content::WebContents* web_contents =
      WebContentsPreferences::GetWebContentsFromProcessID(render_process_id);
  if (!web_contents) {
    std::move(callback).Run(false, false);
    return;
  }
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  if (!permission_helper) {
    std::move(callback).Run(false, false);
    return;
  }
  permission_helper->RequestWebNotificationPermission(
      base::BindOnce(std::move(callback), web_contents->IsAudioMuted()));
}

void AtomBrowserClient::RenderProcessHostDestroyed(
    content::RenderProcessHost* host) {
  int process_id = host->GetID();
  pending_processes_.erase(process_id);
  renderer_is_subframe_.erase(process_id);
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
  if (allowed) {
    platform_util::OpenExternal(
        escaped_url, platform_util::OpenExternalOptions(), base::DoNothing());
  }
}

void HandleExternalProtocolInUI(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    bool has_user_gesture) {
  content::WebContents* web_contents = std::move(web_contents_getter).Run();
  if (!web_contents)
    return;

  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  if (!permission_helper)
    return;

  GURL escaped_url(net::EscapeExternalHandlerValue(url.spec()));
  auto callback = base::BindOnce(&OnOpenExternal, escaped_url);
  permission_helper->RequestOpenExternalPermission(std::move(callback),
                                                   has_user_gesture, url);
}

bool AtomBrowserClient::HandleExternalProtocol(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    int child_id,
    content::NavigationUIData* navigation_data,
    bool is_main_frame,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const base::Optional<url::Origin>& initiating_origin,
    mojo::PendingRemote<network::mojom::URLLoaderFactory>* out_factory) {
  base::PostTask(
      FROM_HERE, {BrowserThread::UI},
      base::BindOnce(&HandleExternalProtocolInUI, url,
                     std::move(web_contents_getter), has_user_gesture));
  return true;
}

std::vector<std::unique_ptr<content::NavigationThrottle>>
AtomBrowserClient::CreateThrottlesForNavigation(
    content::NavigationHandle* handle) {
  std::vector<std::unique_ptr<content::NavigationThrottle>> throttles;
  throttles.push_back(std::make_unique<AtomNavigationThrottle>(handle));

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  throttles.push_back(
      std::make_unique<extensions::ExtensionNavigationThrottle>(handle));
#endif

  return throttles;
}

content::MediaObserver* AtomBrowserClient::GetMediaObserver() {
  return MediaCaptureDevicesDispatcher::GetInstance();
}

content::DevToolsManagerDelegate*
AtomBrowserClient::GetDevToolsManagerDelegate() {
  return new DevToolsManagerDelegate;
}

NotificationPresenter* AtomBrowserClient::GetNotificationPresenter() {
  if (!notification_presenter_) {
    notification_presenter_.reset(NotificationPresenter::Create());
  }
  return notification_presenter_.get();
}

content::PlatformNotificationService*
AtomBrowserClient::GetPlatformNotificationService(
    content::BrowserContext* browser_context) {
  if (!notification_service_) {
    notification_service_ = std::make_unique<PlatformNotificationService>(this);
  }
  return notification_service_.get();
}

base::FilePath AtomBrowserClient::GetDefaultDownloadDirectory() {
  // ~/Downloads
  base::FilePath path;
  if (base::PathService::Get(base::DIR_HOME, &path))
    path = path.Append(FILE_PATH_LITERAL("Downloads"));

  return path;
}

scoped_refptr<network::SharedURLLoaderFactory>
AtomBrowserClient::GetSystemSharedURLLoaderFactory() {
  if (!g_browser_process)
    return nullptr;
  return g_browser_process->shared_url_loader_factory();
}

void AtomBrowserClient::OnNetworkServiceCreated(
    network::mojom::NetworkService* network_service) {
  if (!g_browser_process)
    return;

  g_browser_process->system_network_context_manager()->OnNetworkServiceCreated(
      network_service);
}

std::vector<base::FilePath>
AtomBrowserClient::GetNetworkContextsParentDirectory() {
  base::FilePath user_data_dir;
  base::PathService::Get(DIR_USER_DATA, &user_data_dir);
  DCHECK(!user_data_dir.empty());

  return {user_data_dir};
}

std::string AtomBrowserClient::GetProduct() {
  return "Chrome/" CHROME_VERSION_STRING;
}

std::string AtomBrowserClient::GetUserAgent() {
  if (user_agent_override_.empty())
    return GetApplicationUserAgent();
  return user_agent_override_;
}

void AtomBrowserClient::SetUserAgent(const std::string& user_agent) {
  user_agent_override_ = user_agent;
}

void AtomBrowserClient::RegisterNonNetworkNavigationURLLoaderFactories(
    int frame_tree_node_id,
    NonNetworkURLLoaderFactoryMap* factories) {
  content::WebContents* web_contents =
      content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  api::Protocol* protocol = api::Protocol::FromWrappedClass(
      v8::Isolate::GetCurrent(), web_contents->GetBrowserContext());
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  factories->emplace(
      extensions::kExtensionScheme,
      extensions::CreateExtensionNavigationURLLoaderFactory(
          web_contents->GetBrowserContext(),
          false /* we don't support extensions::WebViewGuest */));
#endif
  if (protocol)
    protocol->RegisterURLLoaderFactories(factories);
}

void AtomBrowserClient::RegisterNonNetworkSubresourceURLLoaderFactories(
    int render_process_id,
    int render_frame_id,
    NonNetworkURLLoaderFactoryMap* factories) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  auto factory = extensions::CreateExtensionURLLoaderFactory(render_process_id,
                                                             render_frame_id);
  if (factory)
    factories->emplace(extensions::kExtensionScheme, std::move(factory));
#endif

  // Chromium may call this even when NetworkService is not enabled.
  content::RenderFrameHost* frame_host =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host);
  if (web_contents) {
    api::Protocol* protocol = api::Protocol::FromWrappedClass(
        v8::Isolate::GetCurrent(), web_contents->GetBrowserContext());
    if (protocol)
      protocol->RegisterURLLoaderFactories(factories);
  }
}

bool AtomBrowserClient::WillCreateURLLoaderFactory(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* frame_host,
    int render_process_id,
    URLLoaderFactoryType type,
    const url::Origin& request_initiator,
    base::Optional<int64_t> navigation_id,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
        header_client,
    bool* bypass_redirect_checks,
    bool* disable_secure_dns,
    network::mojom::URLLoaderFactoryOverridePtr* factory_override) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  api::Protocol* protocol =
      api::Protocol::FromWrappedClass(isolate, browser_context);
  DCHECK(protocol);
  auto web_request = api::WebRequest::FromOrCreate(isolate, browser_context);
  DCHECK(web_request.get());

  auto proxied_receiver = std::move(*factory_receiver);
  mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory_remote;
  *factory_receiver = target_factory_remote.InitWithNewPipeAndPassReceiver();

  // Required by WebRequestInfoInitParams.
  //
  // Note that in Electron we allow webRequest to capture requests sent from
  // browser process, so creation of |navigation_ui_data| is different from
  // Chromium which only does for renderer-initialized navigations.
  std::unique_ptr<extensions::ExtensionNavigationUIData> navigation_ui_data;
  if (navigation_id.has_value()) {
    navigation_ui_data =
        std::make_unique<extensions::ExtensionNavigationUIData>();
  }

  mojo::PendingReceiver<network::mojom::TrustedURLLoaderHeaderClient>
      header_client_receiver;
  if (header_client)
    header_client_receiver = header_client->InitWithNewPipeAndPassReceiver();

  new ProxyingURLLoaderFactory(
      web_request.get(), protocol->intercept_handlers(), browser_context,
      render_process_id, std::move(navigation_ui_data),
      std::move(navigation_id), std::move(proxied_receiver),
      std::move(target_factory_remote), std::move(header_client_receiver),
      type);

  if (bypass_redirect_checks)
    *bypass_redirect_checks = true;
  return true;
}

void AtomBrowserClient::OverrideURLLoaderFactoryParams(
    content::BrowserContext* browser_context,
    const url::Origin& origin,
    bool is_for_isolated_world,
    network::mojom::URLLoaderFactoryParams* factory_params) {
  for (const auto& iter : process_preferences_) {
    if (iter.second.browser_context != browser_context)
      continue;

    if (!iter.second.web_security) {
      // bypass CORB
      factory_params->process_id = iter.first;
      factory_params->is_corb_enabled = false;
    }
  }
}

#if defined(OS_WIN)
bool AtomBrowserClient::PreSpawnRenderer(sandbox::TargetPolicy* policy,
                                         RendererSpawnFlags flags) {
  // Allow crashpad to communicate via named pipe.
  sandbox::ResultCode result = policy->AddRule(
      sandbox::TargetPolicy::SUBSYS_FILES,
      sandbox::TargetPolicy::FILES_ALLOW_ANY, L"\\??\\pipe\\crashpad_*");
  if (result != sandbox::SBOX_ALL_OK)
    return false;
  return true;
}
#endif  // defined(OS_WIN)

bool AtomBrowserClient::BindAssociatedReceiverFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedInterfaceEndpointHandle* handle) {
  if (interface_name == mojom::ElectronAutofillDriver::Name_) {
    AutofillDriverFactory::BindAutofillDriver(
        mojom::ElectronAutofillDriverAssociatedRequest(std::move(*handle)),
        render_frame_host);
    return true;
  }

  return false;
}

std::string AtomBrowserClient::GetApplicationLocale() {
  if (BrowserThread::CurrentlyOn(BrowserThread::IO))
    return g_io_thread_application_locale.Get();
  return *g_application_locale;
}

base::FilePath AtomBrowserClient::GetFontLookupTableCacheDir() {
  base::FilePath user_data_dir;
  base::PathService::Get(DIR_USER_DATA, &user_data_dir);
  DCHECK(!user_data_dir.empty());
  return user_data_dir.Append(FILE_PATH_LITERAL("FontLookupTableCache"));
}

bool AtomBrowserClient::ShouldEnableStrictSiteIsolation() {
  // Enable site isolation. It is off by default in Chromium <= 69.
  return true;
}

void AtomBrowserClient::BindHostReceiverForRenderer(
    content::RenderProcessHost* render_process_host,
    mojo::GenericPendingReceiver receiver) {
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  if (auto host_receiver = receiver.As<spellcheck::mojom::SpellCheckHost>()) {
    SpellCheckHostChromeImpl::Create(render_process_host->GetID(),
                                     std::move(host_receiver));
    return;
  }
#endif
}

void AtomBrowserClient::RegisterBrowserInterfaceBindersForFrame(
    content::RenderFrameHost* render_frame_host,
    service_manager::BinderMapWithContext<content::RenderFrameHost*>* map) {
  map->Add<network_hints::mojom::NetworkHintsHandler>(
      base::BindRepeating(&BindNetworkHintsHandler));
}

std::unique_ptr<content::LoginDelegate> AtomBrowserClient::CreateLoginDelegate(
    const net::AuthChallengeInfo& auth_info,
    content::WebContents* web_contents,
    const content::GlobalRequestID& request_id,
    bool is_main_frame,
    const GURL& url,
    scoped_refptr<net::HttpResponseHeaders> response_headers,
    bool first_auth_attempt,
    LoginAuthRequiredCallback auth_required_callback) {
  return std::make_unique<LoginHandler>(
      auth_info, web_contents, is_main_frame, url, response_headers,
      first_auth_attempt, std::move(auth_required_callback));
}

}  // namespace electron
