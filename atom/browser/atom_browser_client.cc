// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_client.h"

#if defined(OS_WIN)
#include <shlobj.h>
#endif

#include "atom/browser/atom_access_token_store.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_quota_permission_context.h"
#include "atom/browser/atom_resource_dispatcher_host_delegate.h"
#include "atom/browser/atom_speech_recognition_manager_delegate.h"
#include "atom/browser/browser.h"
#include "atom/browser/native_window.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/window_list.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/printing/printing_message_filter.h"
#include "chrome/browser/renderer_host/pepper/chrome_browser_pepper_host_factory.h"
#include "chrome/browser/speech/tts_message_filter.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "net/cert/x509_certificate.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "ppapi/host/ppapi_host.h"
#include "ui/base/l10n/l10n_util.h"
#include "v8/include/v8.h"

namespace atom {

namespace {

// The default routing id of WebContents.
// In Electron each RenderProcessHost only has one WebContents, so this ID is
// same for every WebContents.
int kDefaultRoutingID = 2;

// Next navigation should not restart renderer process.
bool g_suppress_renderer_process_restart = false;

// Custom schemes to be registered to standard.
std::string g_custom_schemes = "";

scoped_refptr<net::X509Certificate> ImportCertFromFile(
    const base::FilePath& path) {
  if (path.empty())
    return nullptr;

  std::string cert_data;
  if (!base::ReadFileToString(path, &cert_data))
    return nullptr;

  net::CertificateList certs =
      net::X509Certificate::CreateCertificateListFromBytes(
          cert_data.data(), cert_data.size(),
          net::X509Certificate::FORMAT_AUTO);

  if (certs.empty())
    return nullptr;

  return certs[0];
}

}  // namespace

// static
void AtomBrowserClient::SuppressRendererProcessRestartForOnce() {
  g_suppress_renderer_process_restart = true;
}

void AtomBrowserClient::SetCustomSchemes(
    const std::vector<std::string>& schemes) {
  g_custom_schemes = JoinString(schemes, ',');
}

AtomBrowserClient::AtomBrowserClient() {
}

AtomBrowserClient::~AtomBrowserClient() {
}

void AtomBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  int process_id = host->GetID();
  host->AddFilter(new printing::PrintingMessageFilter(process_id));
  host->AddFilter(new TtsMessageFilter(process_id, host->GetBrowserContext()));
}

content::SpeechRecognitionManagerDelegate*
    AtomBrowserClient::CreateSpeechRecognitionManagerDelegate() {
  return new AtomSpeechRecognitionManagerDelegate;
}

content::AccessTokenStore* AtomBrowserClient::CreateAccessTokenStore() {
  return new AtomAccessTokenStore;
}

void AtomBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* host, content::WebPreferences* prefs) {
  prefs->javascript_enabled = true;
  prefs->web_security_enabled = true;
  prefs->javascript_can_open_windows_automatically = true;
  prefs->plugins_enabled = true;
  prefs->dom_paste_enabled = true;
  prefs->java_enabled = false;
  prefs->allow_scripts_to_close_windows = true;
  prefs->javascript_can_access_clipboard = true;
  prefs->local_storage_enabled = true;
  prefs->databases_enabled = true;
  prefs->application_cache_enabled = true;
  prefs->allow_universal_access_from_file_urls = true;
  prefs->allow_file_access_from_file_urls = true;
  prefs->experimental_webgl_enabled = true;
  prefs->allow_displaying_insecure_content = false;
  prefs->allow_running_insecure_content = false;

  // Custom preferences of guest page.
  auto web_contents = content::WebContents::FromRenderViewHost(host);
  WebContentsPreferences::OverrideWebkitPrefs(web_contents, prefs);
}

std::string AtomBrowserClient::GetApplicationLocale() {
  return l10n_util::GetApplicationLocale("");
}

void AtomBrowserClient::OverrideSiteInstanceForNavigation(
    content::BrowserContext* browser_context,
    content::SiteInstance* current_instance,
    const GURL& url,
    content::SiteInstance** new_instance) {
  if (g_suppress_renderer_process_restart) {
    g_suppress_renderer_process_restart = false;
    return;
  }

  // Restart renderer process for all navigations except "javacript:" scheme.
  if (url.SchemeIs(url::kJavaScriptScheme))
    return;

  *new_instance = content::SiteInstance::CreateForURL(browser_context, url);

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

  // The registered standard schemes.
  if (!g_custom_schemes.empty())
    command_line->AppendSwitchASCII(switches::kRegisterStandardSchemes,
                                    g_custom_schemes);

#if defined(OS_WIN)
  // Append --app-user-model-id.
  PWSTR current_app_id;
  if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&current_app_id))) {
    command_line->AppendSwitchNative(switches::kAppUserModelId, current_app_id);
    CoTaskMemFree(current_app_id);
  }
#endif

  // If the process is a pending process, we should use the old one.
  if (ContainsKey(pending_processes_, process_id))
    process_id = pending_processes_[process_id];

  // Get the WebContents of the render process.
  content::WebContents* web_contents = content::WebContents::FromRenderViewHost(
      content::RenderViewHost::FromID(process_id, kDefaultRoutingID));
  if (!web_contents)
    return;

  WebContentsPreferences::AppendExtraCommandLineSwitches(
      web_contents, command_line);
}

void AtomBrowserClient::DidCreatePpapiPlugin(
    content::BrowserPpapiHost* host) {
  host->GetPpapiHost()->AddHostFactoryFilter(
      make_scoped_ptr(new chrome::ChromeBrowserPepperHostFactory(host)));
}

content::QuotaPermissionContext*
    AtomBrowserClient::CreateQuotaPermissionContext() {
  return new AtomQuotaPermissionContext;
}

void AtomBrowserClient::SelectClientCertificate(
    content::WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    scoped_ptr<content::ClientCertificateDelegate> delegate) {
  // --client-certificate=`path`
  auto cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch(switches::kClientCertificate)) {
    auto cert_path = cmd->GetSwitchValuePath(switches::kClientCertificate);
    auto certificate = ImportCertFromFile(cert_path);
    if (certificate.get())
      delegate->ContinueWithCertificate(certificate.get());
    return;
  }

  if (!cert_request_info->client_certs.empty())
    Browser::Get()->ClientCertificateSelector(web_contents,
                                              cert_request_info,
                                              delegate.Pass());
}

void AtomBrowserClient::ResourceDispatcherHostCreated() {
  resource_dispatcher_host_delegate_.reset(
      new AtomResourceDispatcherHostDelegate);
  content::ResourceDispatcherHost::Get()->SetDelegate(
      resource_dispatcher_host_delegate_.get());
}

brightray::BrowserMainParts* AtomBrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  v8::V8::Initialize();  // Init V8 before creating main parts.
  return new AtomBrowserMainParts;
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
}

}  // namespace atom
