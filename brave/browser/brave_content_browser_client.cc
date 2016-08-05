// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/browser/brave_content_browser_client.h"

#include "atom/browser/web_contents_permission_helper.h"
#include "atom/browser/web_contents_preferences.h"
#include "brave/browser/notifications/platform_notification_service_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/web_preferences.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_browser_client_extensions_part.h"
// fix for undefined symbol error from io_thread_extension_message_filter.h
#include "components/web_modal/web_contents_modal_dialog_manager.h"
namespace web_modal {
SingleWebContentsDialogManager*
WebContentsModalDialogManager::CreateNativeWebModalManager(
    gfx::NativeWindow dialog,
    SingleWebContentsDialogManagerDelegate* native_delegate) {
  // TODO(bridiver): Investigate if we need to implement this.
  NOTREACHED();
  return NULL;
}
}  // namespace web_modal
#endif


namespace brave {

BraveContentBrowserClient::BraveContentBrowserClient() {
#if defined(ENABLE_EXTENSIONS)
  extensions_part_.reset(new extensions::AtomBrowserClientExtensionsPart);
#endif
}

BraveContentBrowserClient::~BraveContentBrowserClient() {
}

void BraveContentBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  atom::AtomBrowserClient::RenderProcessWillLaunch(host);
#if defined(ENABLE_EXTENSIONS)
  extensions_part_->RenderProcessWillLaunch(host);
#endif
}

void BraveContentBrowserClient::BrowserURLHandlerCreated(
    content::BrowserURLHandler* handler) {
#if defined(ENABLE_EXTENSIONS)
  extensions_part_->BrowserURLHandlerCreated(handler);
#endif
}

std::string BraveContentBrowserClient::GetApplicationLocale() {
  std::string locale;
#if defined(ENABLE_EXTENSIONS)
  locale = extensions_part_->GetApplicationLocale();
#endif
  return locale.empty() ? l10n_util::GetApplicationLocale(locale) : locale;
}

void BraveContentBrowserClient::SetApplicationLocale(std::string locale) {
#if defined(ENABLE_EXTENSIONS)
  extensions::AtomBrowserClientExtensionsPart::SetApplicationLocale(locale);
#endif
}

void BraveContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int process_id) {
  std::string process_type = command_line->GetSwitchValueASCII("type");
  if (process_type != "renderer")
    return;

  atom::AtomBrowserClient::AppendExtraCommandLineSwitches(command_line, process_id);

#if defined(ENABLE_EXTENSIONS)
  content::RenderProcessHost* process =
      content::RenderProcessHost::FromID(process_id);
  extensions_part_->AppendExtraRendererCommandLineSwitches(
        command_line, process, process->GetBrowserContext());
#endif

#if !defined(OS_LINUX)
  if (atom::WebContentsPreferences::run_node(command_line)) {
    // Disable renderer sandbox for most of node's functions.
    command_line->AppendSwitch(::switches::kNoSandbox);
  }
#endif
}

bool BraveContentBrowserClient::CanCreateWindow(
    const GURL& opener_url,
    const GURL& opener_top_level_frame_url,
    const GURL& source_origin,
    WindowContainerType container_type,
    const std::string& frame_name,
    const GURL& target_url,
    const content::Referrer& referrer,
    WindowOpenDisposition disposition,
    const blink::WebWindowFeatures& features,
    bool user_gesture,
    bool opener_suppressed,
    content::ResourceContext* context,
    int render_process_id,
    int opener_render_view_id,
    int opener_render_frame_id,
    bool* no_javascript_access) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  *no_javascript_access = false;

  if (container_type == WINDOW_CONTAINER_TYPE_BACKGROUND) {
    return true;
  }

  // this will override allowpopups we need some way to integerate
  // it so we can turn popup blocking off if desired
  if (!user_gesture) {
    return false;
  }

  return true;
}

content::PlatformNotificationService*
BraveContentBrowserClient::GetPlatformNotificationService() {
  return PlatformNotificationServiceImpl::GetInstance();
}

void BraveContentBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* host, content::WebPreferences* prefs) {
  prefs->hyperlink_auditing_enabled = false;
  // Custom preferences of guest page.
  auto web_contents = content::WebContents::FromRenderViewHost(host);
  atom::WebContentsPreferences::OverrideWebkitPrefs(web_contents, prefs);
  #if defined(ENABLE_EXTENSIONS)
    extensions_part_->OverrideWebkitPrefs(host, prefs);
  #endif
}


void BraveContentBrowserClient::SiteInstanceGotProcess(
    content::SiteInstance* site_instance) {
  DCHECK(site_instance->HasProcess());

  auto browser_context = site_instance->GetBrowserContext();
  if (!browser_context)
    return;

#if defined(ENABLE_EXTENSIONS)
  extensions_part_->SiteInstanceGotProcess(site_instance);
#endif
}

void BraveContentBrowserClient::SiteInstanceDeleting(
    content::SiteInstance* site_instance) {
  if (!site_instance->HasProcess())
    return;

#if defined(ENABLE_EXTENSIONS)
  extensions_part_->SiteInstanceDeleting(site_instance);
#endif
}

bool BraveContentBrowserClient::ShouldUseProcessPerSite(
    content::BrowserContext* browser_context, const GURL& effective_url) {
#if defined(ENABLE_EXTENSIONS)
  return extensions::AtomBrowserClientExtensionsPart::ShouldUseProcessPerSite(
      browser_context, effective_url);
#else
  return false;
#endif
}

void BraveContentBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
        std::vector<std::string>* additional_allowed_schemes) {
  #if defined(ENABLE_EXTENSIONS)
    extensions_part_->
        GetAdditionalAllowedSchemesForFileSystem(additional_allowed_schemes);
  #endif
}

bool BraveContentBrowserClient::ShouldAllowOpenURL(
    content::SiteInstance* site_instance, const GURL& url) {
  GURL from_url = site_instance->GetSiteURL();

#if defined(ENABLE_EXTENSIONS)
  bool result;
  if (extensions::AtomBrowserClientExtensionsPart::ShouldAllowOpenURL(
      site_instance, from_url, url, &result))
    return result;
#endif

  return true;
}

// TODO(bridiver) can OverrideSiteInstanceForNavigation be replaced with this?
bool BraveContentBrowserClient::ShouldSwapBrowsingInstancesForNavigation(
    content::SiteInstance* site_instance,
    const GURL& current_url,
    const GURL& new_url) {
#if defined(ENABLE_EXTENSIONS)
  return extensions::AtomBrowserClientExtensionsPart::
      ShouldSwapBrowsingInstancesForNavigation(
          site_instance, current_url, new_url);
#else
  return false;
#endif
}


}  // namespace brave
