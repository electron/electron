// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_browser_client_extensions_part.h"

#include "atom/browser/api/atom_api_extension.h"
#include "base/command_line.h"
#include "chrome/browser/renderer_host/chrome_extension_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/site_instance.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "extensions/browser/extension_message_filter.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/io_thread_extension_message_filter.h"
#include "extensions/common/constants.h"
#include "extensions/common/manifest_handlers/web_accessible_resources_info.h"
#include "extensions/common/switches.h"

using content::BrowserContext;
using content::BrowserThread;
using content::BrowserURLHandler;
using content::SiteInstance;

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

void AtomBrowserClientExtensionsPart::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  int id = host->GetID();
  BrowserContext* context = host->GetBrowserContext();

  host->AddFilter(new ChromeExtensionMessageFilter(id, context));
  host->AddFilter(new ExtensionMessageFilter(id, context));
  host->AddFilter(new IOThreadExtensionMessageFilter(id, context));
  extension_web_request_api_helpers::SendExtensionWebRequestStatusToHost(host);
}

void AtomBrowserClientExtensionsPart::SiteInstanceGotProcess(
    SiteInstance* site_instance) {
  BrowserContext* context = site_instance->GetProcess()->GetBrowserContext();
  ExtensionRegistry* registry = ExtensionRegistry::Get(context);
  if (!registry)
    return;

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
