// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_browser_client_extensions_part.h"

#include <map>
#include "atom/browser/api/atom_api_extension.h"
#include "atom/common/api/api_messages.h"
#include "base/command_line.h"
#include "brave/browser/brave_browser_context.h"
#include "chrome/browser/renderer_host/chrome_extension_message_filter.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/site_instance.h"
#include "extensions/browser/extension_message_filter.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/io_thread_extension_message_filter.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_l10n_util.h"
#include "extensions/common/manifest_handlers/web_accessible_resources_info.h"
#include "extensions/common/switches.h"

using brave::BraveBrowserContext;
using content::BrowserContext;
using content::BrowserThread;
using content::BrowserURLHandler;
using content::SiteInstance;

namespace {

static std::map<int, void*> render_process_hosts_;

// Cached version of the locale so we can return the locale on the I/O
// thread.
base::LazyInstance<std::string> io_thread_application_locale;

void SetApplicationLocaleOnIOThread(const std::string& locale) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  io_thread_application_locale.Get() = locale;
}

}  // namespace

namespace extensions {

AtomBrowserClientExtensionsPart::AtomBrowserClientExtensionsPart() {
}

AtomBrowserClientExtensionsPart::~AtomBrowserClientExtensionsPart() {
}

// static
bool AtomBrowserClientExtensionsPart::ShouldUseProcessPerSite(
    content::BrowserContext* browser_context, const GURL& effective_url) {
  // return false for non-extension urls
  if (!effective_url.SchemeIs(kExtensionScheme))
    return false;

  ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context);
  if (!registry)
    return false;

  const Extension* extension =
      registry->enabled_extensions().GetByID(effective_url.host());
  if (!extension)
    return false;

  return true;
}

// static
bool AtomBrowserClientExtensionsPart::
    ShouldSwapBrowsingInstancesForNavigation(SiteInstance* site_instance,
                                             const GURL& current_url,
                                             const GURL& new_url) {
  // If we don't have an ExtensionRegistry, then rely on the SiteInstance logic
  // in RenderFrameHostManager to decide when to swap.
  ExtensionRegistry* registry =
      ExtensionRegistry::Get(site_instance->GetBrowserContext());
  if (!registry)
    return false;

  // We must use a new BrowsingInstance (forcing a process swap and disabling
  // scripting by existing tabs) if one of the URLs is an extension and the
  // other is not the exact same extension.
  const Extension* current_extension =
      registry->enabled_extensions().GetExtensionOrAppByURL(current_url);
  if (current_extension && current_extension->is_hosted_app())
    current_extension = NULL;

  const Extension* new_extension =
      registry->enabled_extensions().GetExtensionOrAppByURL(new_url);
  if (new_extension && new_extension->is_hosted_app())
    new_extension = NULL;

  // First do a process check.  We should force a BrowsingInstance swap if the
  // current process doesn't know about new_extension, even if current_extension
  // is somehow the same as new_extension.
  ProcessMap* process_map = ProcessMap::Get(site_instance->GetBrowserContext());
  if (new_extension &&
      site_instance->HasProcess() &&
      !process_map->Contains(
          new_extension->id(), site_instance->GetProcess()->GetID()))
    return true;

  // Otherwise, swap BrowsingInstances if current_extension and new_extension
  // differ.
  return current_extension != new_extension;
}

// static
bool AtomBrowserClientExtensionsPart::ShouldAllowOpenURL(
    content::SiteInstance* site_instance,
    const GURL& from_url,
    const GURL& to_url,
    bool* result) {
  DCHECK(result);

  // Do not allow pages from the web or other extensions navigate to
  // non-web-accessible extension resources.
  if (to_url.SchemeIs(kExtensionScheme) &&
      (from_url.SchemeIsHTTPOrHTTPS() || from_url.SchemeIs(kExtensionScheme))) {
    content::BrowserContext* browser_context =
        site_instance->GetProcess()->GetBrowserContext();
    ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context);
    if (!registry) {
      *result = true;
      return true;
    }
    const Extension* extension =
        registry->enabled_extensions().GetExtensionOrAppByURL(to_url);
    if (!extension) {
      *result = true;
      return true;
    }
    const Extension* from_extension =
        registry->enabled_extensions().GetExtensionOrAppByURL(
            site_instance->GetSiteURL());
    if (from_extension && from_extension->id() == extension->id()) {
      *result = true;
      return true;
    }

    if (!WebAccessibleResourcesInfo::IsResourceWebAccessible(
            extension, to_url.path())) {
      *result = false;
      return true;
    }
  }
  return false;
}

std::string AtomBrowserClientExtensionsPart::GetApplicationLocale() {
  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    return io_thread_application_locale.Get();
  } else {
    return extension_l10n_util::CurrentLocaleOrDefault();
  }
}

// static
void AtomBrowserClientExtensionsPart::SetApplicationLocale(std::string locale) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // This object is guaranteed to outlive all threads so we don't have to
  // worry about the lack of refcounting and can just post as Unretained.
  //
  // The common case is that this function is called early in Chrome startup
  // before any threads are created (it will also be called later if the user
  // changes the pref). In this case, there will be no threads created and
  // posting will fail. When there are no threads, we can just set the string
  // without worrying about threadsafety.
  if (!BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
          base::Bind(&SetApplicationLocaleOnIOThread, locale))) {
    io_thread_application_locale.Get() = locale;
  }
  extension_l10n_util::SetProcessLocale(locale);
}

// static
void AtomBrowserClientExtensionsPart::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref("content_settings");
  registry->RegisterStringPref(prefs::kApplicationLocale,
      extension_l10n_util::CurrentLocaleOrDefault());
}

void AtomBrowserClientExtensionsPart::OverrideWebkitPrefs(
    content::RenderViewHost* host, content::WebPreferences* prefs) {
  host->Send(new AtomMsg_UpdateWebKitPrefs(*prefs));
}

void AtomBrowserClientExtensionsPart::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  int id = host->GetID();
  auto context =
      BraveBrowserContext::FromBrowserContext(host->GetBrowserContext());

  host->AddFilter(new ChromeExtensionMessageFilter(id, context));
  host->AddFilter(new ExtensionMessageFilter(id, context));
  host->AddFilter(new IOThreadExtensionMessageFilter(id, context));
  extension_web_request_api_helpers::SendExtensionWebRequestStatusToHost(host);

  auto user_prefs_registrar = context->user_prefs_change_registrar();
  if (!user_prefs_registrar->IsObserved("content_settings")) {
    user_prefs_registrar->Add(
        "content_settings",
        base::Bind(&AtomBrowserClientExtensionsPart::UpdateContentSettings,
                   base::Unretained(this)));
  }
  UpdateContentSettingsForHost(host->GetID());
}

void AtomBrowserClientExtensionsPart::UpdateContentSettingsForHost(
  int render_process_id) {
  auto host = content::RenderProcessHost::FromID(render_process_id);
  if (!host)
    return;

  auto context =
      static_cast<atom::AtomBrowserContext*>(host->GetBrowserContext());
  auto user_prefs = user_prefs::UserPrefs::Get(context);
  const base::DictionaryValue* content_settings =
    user_prefs->GetDictionary("content_settings");
  host->Send(new AtomMsg_UpdateContentSettings(*content_settings));
}

void AtomBrowserClientExtensionsPart::UpdateContentSettings() {
  for (std::map<int, void*>::iterator
      it = render_process_hosts_.begin();
      it != render_process_hosts_.end();
      ++it) {
    UpdateContentSettingsForHost(it->first);
  }
}

void AtomBrowserClientExtensionsPart::SiteInstanceGotProcess(
    SiteInstance* site_instance) {
  BrowserContext* context = site_instance->GetProcess()->GetBrowserContext();
  ExtensionRegistry* registry = ExtensionRegistry::Get(context);
  if (!registry)
    return;

  render_process_hosts_[site_instance->GetProcess()->GetID()] = nullptr;

  const Extension* extension =
      registry->enabled_extensions().GetExtensionOrAppByURL(
          site_instance->GetSiteURL());
  if (!extension)
    return;

  ProcessMap::Get(context)->Insert(extension->id(),
                                   site_instance->GetProcess()->GetID(),
                                   site_instance->GetId());

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&InfoMap::RegisterExtensionProcess,
                 ExtensionSystem::Get(context)->info_map(), extension->id(),
                 site_instance->GetProcess()->GetID(), site_instance->GetId()));
}

void AtomBrowserClientExtensionsPart::SiteInstanceDeleting(
    SiteInstance* site_instance) {
  BrowserContext* context = site_instance->GetBrowserContext();
  ExtensionRegistry* registry = ExtensionRegistry::Get(context);
  if (!registry)
    return;

  if (render_process_hosts_[site_instance->GetProcess()->GetID()])
    render_process_hosts_.erase(site_instance->GetProcess()->GetID());

  const Extension* extension =
      registry->enabled_extensions().GetExtensionOrAppByURL(
          site_instance->GetSiteURL());
  if (!extension)
    return;

  ProcessMap::Get(context)->Remove(extension->id(),
                                   site_instance->GetProcess()->GetID(),
                                   site_instance->GetId());

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&InfoMap::UnregisterExtensionProcess,
                 ExtensionSystem::Get(context)->info_map(), extension->id(),
                 site_instance->GetProcess()->GetID(), site_instance->GetId()));
}

void AtomBrowserClientExtensionsPart::BrowserURLHandlerCreated(
    BrowserURLHandler* handler) {
  handler->AddHandlerPair(&atom::api::Extension::HandleURLOverride,
                          BrowserURLHandler::null_handler());
  handler->AddHandlerPair(BrowserURLHandler::null_handler(),
                          &atom::api::Extension::HandleURLOverrideReverse);
}

void AtomBrowserClientExtensionsPart::
    GetAdditionalAllowedSchemesForFileSystem(
        std::vector<std::string>* additional_allowed_schemes) {
  additional_allowed_schemes->push_back(kExtensionScheme);
}

void AtomBrowserClientExtensionsPart::
    AppendExtraRendererCommandLineSwitches(base::CommandLine* command_line,
                                           content::RenderProcessHost* process,
                                           BrowserContext* context) {
  if (!process)
    return;
  DCHECK(context);
  if (ProcessMap::Get(context)->Contains(process->GetID())) {
    command_line->AppendSwitch(switches::kExtensionProcess);
  }
}

}  // namespace extensions
