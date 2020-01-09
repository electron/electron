// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/atom_extensions_browser_client.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/task/post_task.h"
#include "build/build_config.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/user_agent.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/component_extension_resource_manager.h"
#include "extensions/browser/core_extensions_browser_api_provider.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/null_app_sorting.h"
#include "extensions/browser/updater/null_extension_cache.h"
#include "extensions/browser/url_request_util.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_url_handlers.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "shell/browser/atom_browser_client.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/browser/browser.h"
#include "shell/browser/extensions/api/runtime/atom_runtime_api_delegate.h"
#include "shell/browser/extensions/atom_extension_host_delegate.h"
#include "shell/browser/extensions/atom_extension_system_factory.h"
#include "shell/browser/extensions/atom_extension_web_contents_observer.h"
#include "shell/browser/extensions/atom_navigation_ui_data.h"
#include "shell/browser/extensions/electron_extensions_api_client.h"
#include "shell/browser/extensions/electron_extensions_browser_api_provider.h"
#include "shell/browser/extensions/electron_process_manager_delegate.h"

using content::BrowserContext;
using content::BrowserThread;

namespace electron {

AtomExtensionsBrowserClient::AtomExtensionsBrowserClient()
    : api_client_(new extensions::ElectronExtensionsAPIClient),
      process_manager_delegate_(new extensions::ElectronProcessManagerDelegate),
      extension_cache_(new extensions::NullExtensionCache()) {
  // Electron does not have a concept of channel, so leave UNKNOWN to
  // enable all channel-dependent extension APIs.
  extensions::SetCurrentChannel(version_info::Channel::UNKNOWN);

  AddAPIProvider(
      std::make_unique<extensions::CoreExtensionsBrowserAPIProvider>());
  AddAPIProvider(
      std::make_unique<extensions::ElectronExtensionsBrowserAPIProvider>());
}

AtomExtensionsBrowserClient::~AtomExtensionsBrowserClient() {}

bool AtomExtensionsBrowserClient::IsShuttingDown() {
  return electron::Browser::Get()->is_shutting_down();
}

bool AtomExtensionsBrowserClient::AreExtensionsDisabled(
    const base::CommandLine& command_line,
    BrowserContext* context) {
  return false;
}

bool AtomExtensionsBrowserClient::IsValidContext(BrowserContext* context) {
  auto context_map = AtomBrowserContext::browser_context_map();
  for (auto const& entry : context_map) {
    if (entry.second && entry.second.get() == context)
      return true;
  }
  return false;
}

bool AtomExtensionsBrowserClient::IsSameContext(BrowserContext* first,
                                                BrowserContext* second) {
  return first == second;
}

bool AtomExtensionsBrowserClient::HasOffTheRecordContext(
    BrowserContext* context) {
  return false;
}

BrowserContext* AtomExtensionsBrowserClient::GetOffTheRecordContext(
    BrowserContext* context) {
  // app_shell only supports a single context.
  return nullptr;
}

BrowserContext* AtomExtensionsBrowserClient::GetOriginalContext(
    BrowserContext* context) {
  DCHECK(context);
  if (context->IsOffTheRecord()) {
    return AtomBrowserContext::From("", false).get();
  } else {
    return context;
  }
}

bool AtomExtensionsBrowserClient::IsGuestSession(
    BrowserContext* context) const {
  return false;
}

bool AtomExtensionsBrowserClient::IsExtensionIncognitoEnabled(
    const std::string& extension_id,
    content::BrowserContext* context) const {
  return false;
}

bool AtomExtensionsBrowserClient::CanExtensionCrossIncognito(
    const extensions::Extension* extension,
    content::BrowserContext* context) const {
  return false;
}

base::FilePath AtomExtensionsBrowserClient::GetBundleResourcePath(
    const network::ResourceRequest& request,
    const base::FilePath& extension_resources_path,
    int* resource_id) const {
  *resource_id = 0;
  return base::FilePath();
}

void AtomExtensionsBrowserClient::LoadResourceFromResourceBundle(
    const network::ResourceRequest& request,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    const base::FilePath& resource_relative_path,
    int resource_id,
    const std::string& content_security_policy,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    bool send_cors_header) {
  NOTREACHED() << "Load resources from bundles not supported.";
}

namespace {
bool AllowCrossRendererResourceLoad(const GURL& url,
                                    content::ResourceType resource_type,
                                    ui::PageTransition page_transition,
                                    int child_id,
                                    bool is_incognito,
                                    const extensions::Extension* extension,
                                    const extensions::ExtensionSet& extensions,
                                    const extensions::ProcessMap& process_map,
                                    bool* allowed) {
  if (extensions::url_request_util::AllowCrossRendererResourceLoad(
          url, resource_type, page_transition, child_id, is_incognito,
          extension, extensions, process_map, allowed)) {
    return true;
  }

  // If there aren't any explicitly marked web accessible resources, the
  // load should be allowed only if it is by DevTools. A close approximation is
  // checking if the extension contains a DevTools page.
  if (extension && !extensions::ManifestURL::Get(
                        extension, extensions::manifest_keys::kDevToolsPage)
                        .is_empty()) {
    *allowed = true;
    return true;
  }

  // Couldn't determine if the resource is allowed or not.
  return false;
}
}  // namespace

bool AtomExtensionsBrowserClient::AllowCrossRendererResourceLoad(
    const GURL& url,
    content::ResourceType resource_type,
    ui::PageTransition page_transition,
    int child_id,
    bool is_incognito,
    const extensions::Extension* extension,
    const extensions::ExtensionSet& extensions,
    const extensions::ProcessMap& process_map) {
  bool allowed = false;
  if (::electron::AllowCrossRendererResourceLoad(
          url, resource_type, page_transition, child_id, is_incognito,
          extension, extensions, process_map, &allowed)) {
    return allowed;
  }

  // Couldn't determine if resource is allowed. Block the load.
  return false;
}

PrefService* AtomExtensionsBrowserClient::GetPrefServiceForContext(
    BrowserContext* context) {
  return static_cast<AtomBrowserContext*>(context)->prefs();
}

void AtomExtensionsBrowserClient::GetEarlyExtensionPrefsObservers(
    content::BrowserContext* context,
    std::vector<extensions::EarlyExtensionPrefsObserver*>* observers) const {}

extensions::ProcessManagerDelegate*
AtomExtensionsBrowserClient::GetProcessManagerDelegate() const {
  return process_manager_delegate_.get();
}

std::unique_ptr<extensions::ExtensionHostDelegate> AtomExtensionsBrowserClient::
    CreateExtensionHostDelegate() {  // TODO(samuelmaddock):
  return base::WrapUnique(new extensions::AtomExtensionHostDelegate);
}

bool AtomExtensionsBrowserClient::DidVersionUpdate(BrowserContext* context) {
  // TODO(jamescook): We might want to tell extensions when app_shell updates.
  return false;
}

void AtomExtensionsBrowserClient::PermitExternalProtocolHandler() {}

bool AtomExtensionsBrowserClient::IsInDemoMode() {
  return false;
}

bool AtomExtensionsBrowserClient::IsScreensaverInDemoMode(
    const std::string& app_id) {
  return false;
}

bool AtomExtensionsBrowserClient::IsRunningInForcedAppMode() {
  return false;
}

bool AtomExtensionsBrowserClient::IsAppModeForcedForApp(
    const extensions::ExtensionId& extension_id) {
  return false;
}

bool AtomExtensionsBrowserClient::IsLoggedInAsPublicAccount() {
  return false;
}

extensions::ExtensionSystemProvider*
AtomExtensionsBrowserClient::GetExtensionSystemFactory() {
  return extensions::AtomExtensionSystemFactory::GetInstance();
}

void AtomExtensionsBrowserClient::RegisterExtensionInterfaces(
    service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>*
        registry,
    content::RenderFrameHost* render_frame_host,
    const extensions::Extension* extension) const {}

std::unique_ptr<extensions::RuntimeAPIDelegate>
AtomExtensionsBrowserClient::CreateRuntimeAPIDelegate(
    content::BrowserContext* context) const {
  return std::make_unique<extensions::AtomRuntimeAPIDelegate>(context);
}

const extensions::ComponentExtensionResourceManager*
AtomExtensionsBrowserClient::GetComponentExtensionResourceManager() {
  return NULL;
}

void AtomExtensionsBrowserClient::BroadcastEventToRenderers(
    extensions::events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::ListValue> args,
    bool dispatch_to_off_the_record_profiles) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    base::PostTask(
        FROM_HERE, {BrowserThread::UI},
        base::BindOnce(&AtomExtensionsBrowserClient::BroadcastEventToRenderers,
                       base::Unretained(this), histogram_value, event_name,
                       std::move(args), dispatch_to_off_the_record_profiles));
    return;
  }

  std::unique_ptr<extensions::Event> event(
      new extensions::Event(histogram_value, event_name, std::move(args)));
  auto context_map = AtomBrowserContext::browser_context_map();
  for (auto const& entry : context_map) {
    if (entry.second) {
      extensions::EventRouter::Get(entry.second.get())
          ->BroadcastEvent(std::move(event));
    }
  }
}

extensions::ExtensionCache* AtomExtensionsBrowserClient::GetExtensionCache() {
  return extension_cache_.get();
}

bool AtomExtensionsBrowserClient::IsBackgroundUpdateAllowed() {
  return true;
}

bool AtomExtensionsBrowserClient::IsMinBrowserVersionSupported(
    const std::string& min_version) {
  return true;
}

void AtomExtensionsBrowserClient::SetAPIClientForTest(
    extensions::ExtensionsAPIClient* api_client) {
  api_client_.reset(api_client);
}

extensions::ExtensionWebContentsObserver*
AtomExtensionsBrowserClient::GetExtensionWebContentsObserver(
    content::WebContents* web_contents) {
  return extensions::AtomExtensionWebContentsObserver::FromWebContents(
      web_contents);
}

extensions::KioskDelegate* AtomExtensionsBrowserClient::GetKioskDelegate() {
  return nullptr;
}

bool AtomExtensionsBrowserClient::IsLockScreenContext(
    content::BrowserContext* context) {
  return false;
}

std::string AtomExtensionsBrowserClient::GetApplicationLocale() {
  return AtomBrowserClient::Get()->GetApplicationLocale();
}

std::string AtomExtensionsBrowserClient::GetUserAgent() const {
  return AtomBrowserClient::Get()->GetUserAgent();
}

void AtomExtensionsBrowserClient::RegisterBrowserInterfaceBindersForFrame(
    service_manager::BinderMapWithContext<content::RenderFrameHost*>* map,
    content::RenderFrameHost* render_frame_host,
    const extensions::Extension* extension) const {}

}  // namespace electron
