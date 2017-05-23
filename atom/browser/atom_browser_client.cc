// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_client.h"

#if defined(OS_WIN)
#include <shlobj.h>
#endif

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/api/atom_api_protocol.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_quota_permission_context.h"
#include "atom/browser/atom_resource_dispatcher_host_delegate.h"
#include "atom/browser/atom_speech_recognition_manager_delegate.h"
#include "atom/browser/child_web_contents_tracker.h"
#include "atom/browser/native_window.h"
#include "atom/browser/web_contents_permission_helper.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/window_list.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "chrome/browser/printing/printing_message_filter.h"
#include "chrome/browser/renderer_host/pepper/chrome_browser_pepper_host_factory.h"
#include "chrome/browser/renderer_host/pepper/widevine_cdm_message_filter.h"
#include "chrome/browser/speech/tts_message_filter.h"
#include "content/common/resource_request_body_impl.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/web_preferences.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "ppapi/host/ppapi_host.h"
#include "ui/base/l10n/l10n_util.h"
#include "v8/include/v8.h"

namespace atom {

namespace {

// Next navigation should not restart renderer process.
bool g_suppress_renderer_process_restart = false;

// Custom schemes to be registered to handle service worker.
std::string g_custom_service_worker_schemes = "";

void Noop(scoped_refptr<content::SiteInstance>) {
}

}  // namespace

// static
void AtomBrowserClient::SuppressRendererProcessRestartForOnce() {
  g_suppress_renderer_process_restart = true;
}

void AtomBrowserClient::SetCustomServiceWorkerSchemes(
    const std::vector<std::string>& schemes) {
  g_custom_service_worker_schemes = base::JoinString(schemes, ",");
}

AtomBrowserClient::AtomBrowserClient() : delegate_(nullptr) {
}

AtomBrowserClient::~AtomBrowserClient() {
}

content::WebContents* AtomBrowserClient::GetWebContentsFromProcessID(
    int process_id) {
  // If the process is a pending process, we should use the old one.
  if (base::ContainsKey(pending_processes_, process_id))
    process_id = pending_processes_[process_id];

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
    auto web_contents =
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
  return
      !content::SiteInstance::IsSameWebSite(browser_context, src_url, url) &&
      // `IsSameWebSite` doesn't seem to work for some URIs such as `file:`,
      // handle these scenarios by comparing only the site as defined by
      // `GetSiteForURL`.
      content::SiteInstance::GetSiteForURL(browser_context, url) != src_url;
}

void AtomBrowserClient::AddProcessPreferences(
    int process_id, AtomBrowserClient::ProcessPreferences prefs) {
  base::AutoLock auto_lock(process_preferences_lock_);
  process_preferences_[process_id] = prefs;
}

void AtomBrowserClient::RemoveProcessPreferences(int process_id) {
  base::AutoLock auto_lock(process_preferences_lock_);
  process_preferences_.erase(process_id);
}

bool AtomBrowserClient::IsRendererSandboxed(int process_id) {
  base::AutoLock auto_lock(process_preferences_lock_);
  auto it = process_preferences_.find(process_id);
  return it != process_preferences_.end() && it->second.sandbox;
}

bool AtomBrowserClient::RendererUsesNativeWindowOpen(int process_id) {
  base::AutoLock auto_lock(process_preferences_lock_);
  auto it = process_preferences_.find(process_id);
  return it != process_preferences_.end() && it->second.native_window_open;
}

bool AtomBrowserClient::RendererDisablesPopups(int process_id) {
  base::AutoLock auto_lock(process_preferences_lock_);
  auto it = process_preferences_.find(process_id);
  return it != process_preferences_.end() && it->second.disable_popups;
}

void AtomBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  int process_id = host->GetID();
  host->AddFilter(new printing::PrintingMessageFilter(process_id));
  host->AddFilter(new TtsMessageFilter(process_id, host->GetBrowserContext()));
  host->AddFilter(
      new WidevineCdmMessageFilter(process_id, host->GetBrowserContext()));

  content::WebContents* web_contents = GetWebContentsFromProcessID(process_id);
  ProcessPreferences process_prefs;
  process_prefs.sandbox = WebContentsPreferences::IsSandboxed(web_contents);
  process_prefs.native_window_open
      = WebContentsPreferences::UsesNativeWindowOpen(web_contents);
  process_prefs.disable_popups
      = WebContentsPreferences::DisablePopups(web_contents);
  AddProcessPreferences(host->GetID(), process_prefs);
  // ensure the ProcessPreferences is removed later
  host->AddObserver(this);
}

content::SpeechRecognitionManagerDelegate*
    AtomBrowserClient::CreateSpeechRecognitionManagerDelegate() {
  return new AtomSpeechRecognitionManagerDelegate;
}

void AtomBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* host, content::WebPreferences* prefs) {
  prefs->javascript_enabled = true;
  prefs->web_security_enabled = true;
  prefs->javascript_can_open_windows_automatically = true;
  prefs->plugins_enabled = true;
  prefs->dom_paste_enabled = true;
  prefs->allow_scripts_to_close_windows = true;
  prefs->javascript_can_access_clipboard = true;
  prefs->local_storage_enabled = true;
  prefs->databases_enabled = true;
  prefs->application_cache_enabled = true;
  prefs->allow_universal_access_from_file_urls = true;
  prefs->allow_file_access_from_file_urls = true;
  prefs->experimental_webgl_enabled = true;
  prefs->allow_running_insecure_content = false;

  // Custom preferences of guest page.
  auto web_contents = content::WebContents::FromRenderViewHost(host);
  WebContentsPreferences::OverrideWebkitPrefs(web_contents, prefs);
}

std::string AtomBrowserClient::GetApplicationLocale() {
  return l10n_util::GetApplicationLocale("");
}

void AtomBrowserClient::OverrideSiteInstanceForNavigation(
    content::RenderFrameHost* render_frame_host,
    content::BrowserContext* browser_context,
    content::SiteInstance* current_instance,
    const GURL& url,
    content::SiteInstance** new_instance) {
  if (g_suppress_renderer_process_restart) {
    g_suppress_renderer_process_restart = false;
    return;
  }

  if (!ShouldCreateNewSiteInstance(render_frame_host, browser_context,
                                   current_instance, url))
    return;

  scoped_refptr<content::SiteInstance> site_instance =
      content::SiteInstance::CreateForURL(browser_context, url);
  *new_instance = site_instance.get();

  // Make sure the |site_instance| is not freed when this function returns.
  // FIXME(zcbenz): We should adjust OverrideSiteInstanceForNavigation's
  // interface to solve this.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&Noop, base::RetainedRef(site_instance)));

  // Remember the original renderer process of the pending renderer process.
  auto current_process = current_instance->GetProcess();
  auto pending_process = (*new_instance)->GetProcess();
  pending_processes_[pending_process->GetID()] = current_process->GetID();
  // Clear the entry in map when process ends.
  current_process->AddObserver(this);
}

void AtomBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int process_id) {
  std::string process_type = command_line->GetSwitchValueASCII("type");
  if (process_type != "renderer")
    return;

  // Copy following switches to child process.
  static const char* const kCommonSwitchNames[] = {
    switches::kStandardSchemes,
    switches::kEnableSandbox,
    switches::kSecureSchemes
  };
  command_line->CopySwitchesFrom(
      *base::CommandLine::ForCurrentProcess(),
      kCommonSwitchNames, arraysize(kCommonSwitchNames));

  // The registered service worker schemes.
  if (!g_custom_service_worker_schemes.empty())
    command_line->AppendSwitchASCII(switches::kRegisterServiceWorkerSchemes,
                                    g_custom_service_worker_schemes);

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
  if (!web_contents)
    return;

  WebContentsPreferences::AppendExtraCommandLineSwitches(
      web_contents, command_line);
}

void AtomBrowserClient::DidCreatePpapiPlugin(
    content::BrowserPpapiHost* host) {
  host->GetPpapiHost()->AddHostFactoryFilter(
      base::WrapUnique(new chrome::ChromeBrowserPepperHostFactory(host)));
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
    bool overridable,
    bool strict_enforcement,
    bool expired_previous_decision,
    const base::Callback<void(content::CertificateRequestResultType)>&
        callback) {
  if (delegate_) {
    delegate_->AllowCertificateError(
        web_contents, cert_error, ssl_info, request_url,
        resource_type, overridable, strict_enforcement,
        expired_previous_decision, callback);
  }
}

void AtomBrowserClient::SelectClientCertificate(
    content::WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    std::unique_ptr<content::ClientCertificateDelegate> delegate) {
  if (!cert_request_info->client_certs.empty() && delegate_) {
    delegate_->SelectClientCertificate(
        web_contents, cert_request_info, std::move(delegate));
  }
}

void AtomBrowserClient::ResourceDispatcherHostCreated() {
  resource_dispatcher_host_delegate_.reset(
      new AtomResourceDispatcherHostDelegate);
  content::ResourceDispatcherHost::Get()->SetDelegate(
      resource_dispatcher_host_delegate_.get());
}

bool AtomBrowserClient::CanCreateWindow(
    int opener_render_process_id,
    int opener_render_frame_id,
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
    const scoped_refptr<content::ResourceRequestBodyImpl>& body,
    bool user_gesture,
    bool opener_suppressed,
    content::ResourceContext* context,
    bool* no_javascript_access) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

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
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
        base::Bind(&api::App::OnCreateWindow,
                   base::Unretained(static_cast<api::App*>(delegate_)),
                                    target_url,
                                    frame_name,
                                    disposition,
                                    additional_features,
                                    body,
                                    opener_render_process_id,
                                    opener_render_frame_id));
  }

  return false;
}

void AtomBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
    std::vector<std::string>* additional_schemes) {
  auto schemes_list = api::GetStandardSchemes();
  if (!schemes_list.empty())
    additional_schemes->insert(additional_schemes->end(),
                               schemes_list.begin(),
                               schemes_list.end());
  additional_schemes->push_back(content::kChromeDevToolsScheme);
}

brightray::BrowserMainParts* AtomBrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  v8::V8::Initialize();  // Init V8 before creating main parts.
  return new AtomBrowserMainParts;
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
  auto permission_helper =
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
  for (const auto& entry : pending_processes_) {
    if (entry.first == process_id || entry.second == process_id) {
      pending_processes_.erase(entry.first);
      break;
    }
  }
  RemoveProcessPreferences(process_id);
}

void AtomBrowserClient::RenderProcessReady(content::RenderProcessHost* host) {
  render_process_host_pids_[host->GetID()] = base::GetProcId(host->GetHandle());
  if (delegate_) {
    static_cast<api::App*>(delegate_)->RenderProcessReady(host);
  }
}

void AtomBrowserClient::RenderProcessExited(content::RenderProcessHost* host,
                                            base::TerminationStatus status,
                                            int exit_code) {
  auto host_pid = render_process_host_pids_.find(host->GetID());
  if (host_pid != render_process_host_pids_.end()) {
    if (delegate_) {
      static_cast<api::App*>(delegate_)->RenderProcessDisconnected(
          host_pid->second);
    }
    render_process_host_pids_.erase(host_pid);
  }
}

}  // namespace atom
