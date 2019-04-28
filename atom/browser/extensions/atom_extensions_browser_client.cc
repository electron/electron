// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extensions_browser_client.h"

#include <utility>

#include "atom/browser/extensions/api/runtime/atom_runtime_api_delegate.h"
#include "atom/browser/extensions/atom_extension_host_delegate.h"
#include "atom/browser/extensions/atom_extension_system_factory.h"
#include "atom/browser/extensions/atom_extension_web_contents_observer.h"
#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/task/post_task.h"
#include "build/build_config.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/user_agent.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/component_extension_resource_manager.h"
// #include "extensions/browser/core_extensions_browser_api_provider.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/mojo/interface_registration.h"
#include "extensions/browser/null_app_sorting.h"
#include "extensions/browser/updater/null_extension_cache.h"
#include "extensions/browser/url_request_util.h"
#include "extensions/common/features/feature_channel.h"
// #include "atom/browser/extensions/atom_extensions_api_client.h"
// #include "atom/browser/extensions/atom_extensions_browser_api_provider.h"
#include "atom/browser/extensions/atom_navigation_ui_data.h"
#include "services/network/public/mojom/url_loader.mojom.h"

using content::BrowserContext;
using content::BrowserThread;

namespace atom {

AtomExtensionsBrowserClient::AtomExtensionsBrowserClient()
    : api_client_(new extensions::ExtensionsAPIClient),
      // : api_client_(new extensions::AtomExtensionsAPIClient),
      extension_cache_(new extensions::NullExtensionCache()) {
  // app_shell does not have a concept of channel yet, so leave UNKNOWN to
  // enable all channel-dependent extension APIs.
  extensions::SetCurrentChannel(version_info::Channel::UNKNOWN);

  // TODO(samuelmaddock): undefined symbol for
  // extensions::CoreExtensionsBrowserAPIProvider AddAPIProvider(
  //     std::make_unique<extensions::CoreExtensionsBrowserAPIProvider>());
  // AddAPIProvider(std::make_unique<AtomExtensionsBrowserAPIProvider>());
}

AtomExtensionsBrowserClient::~AtomExtensionsBrowserClient() {}

bool AtomExtensionsBrowserClient::IsShuttingDown() {
  return atom::Browser::Get()->IsShuttingDown();
}

bool AtomExtensionsBrowserClient::AreExtensionsDisabled(
    const base::CommandLine& command_line,
    BrowserContext* context) {
  return false;
}

bool AtomExtensionsBrowserClient::IsValidContext(BrowserContext* context) {
  DCHECK(browser_context_);
  return context == browser_context_;
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
  return NULL;
}

BrowserContext* AtomExtensionsBrowserClient::GetOriginalContext(
    BrowserContext* context) {
  return context;
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

net::URLRequestJob*
AtomExtensionsBrowserClient::MaybeCreateResourceBundleRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const base::FilePath& directory_path,
    const std::string& content_security_policy,
    bool send_cors_header) {
  return NULL;
}

base::FilePath AtomExtensionsBrowserClient::GetBundleResourcePath(
    const network::ResourceRequest& request,
    const base::FilePath& extension_resources_path,
    extensions::ComponentExtensionResourceInfo* resource_info) const {
  *resource_info = {};
  return base::FilePath();
}

void AtomExtensionsBrowserClient::LoadResourceFromResourceBundle(
    const network::ResourceRequest& request,
    network::mojom::URLLoaderRequest loader,
    const base::FilePath& resource_relative_path,
    const extensions::ComponentExtensionResourceInfo& resource_info,
    const std::string& content_security_policy,
    network::mojom::URLLoaderClientPtr client,
    bool send_cors_header) {
  NOTREACHED() << "Load resources from bundles not supported.";
}

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
  if (extensions::url_request_util::AllowCrossRendererResourceLoad(
          url, resource_type, page_transition, child_id, is_incognito,
          extension, extensions, process_map, &allowed)) {
    return allowed;
  }

  // Couldn't determine if resource is allowed. Block the load.
  return false;
}

PrefService* AtomExtensionsBrowserClient::GetPrefServiceForContext(
    BrowserContext* context) {
  DCHECK(pref_service_);
  return pref_service_;
}

void AtomExtensionsBrowserClient::GetEarlyExtensionPrefsObservers(
    content::BrowserContext* context,
    std::vector<extensions::ExtensionPrefsObserver*>* observers) const {}

extensions::ProcessManagerDelegate*
AtomExtensionsBrowserClient::GetProcessManagerDelegate() const {
  return NULL;
}

std::unique_ptr<extensions::ExtensionHostDelegate>
AtomExtensionsBrowserClient::CreateExtensionHostDelegate() {
  // TODO(samuelmaddock):
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
  // TODO(samuelmaddock):
  return extensions::AtomExtensionSystemFactory::GetInstance();
  // return extensions::ExtensionSystemFactory::GetInstance();
}

void AtomExtensionsBrowserClient::RegisterExtensionInterfaces(
    service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>*
        registry,
    content::RenderFrameHost* render_frame_host,
    const extensions::Extension* extension) const {
  RegisterInterfacesForExtension(registry, render_frame_host, extension);
}

std::unique_ptr<extensions::RuntimeAPIDelegate>
AtomExtensionsBrowserClient::CreateRuntimeAPIDelegate(
    content::BrowserContext* context) const {
  // TODO(samuelmaddock):
  return std::make_unique<extensions::AtomRuntimeAPIDelegate>(context);
}

const extensions::ComponentExtensionResourceManager*
AtomExtensionsBrowserClient::GetComponentExtensionResourceManager() {
  return NULL;
}

void AtomExtensionsBrowserClient::BroadcastEventToRenderers(
    extensions::events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::ListValue> args) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    base::PostTaskWithTraits(
        FROM_HERE, {BrowserThread::UI},
        base::BindOnce(&AtomExtensionsBrowserClient::BroadcastEventToRenderers,
                       base::Unretained(this), histogram_value, event_name,
                       std::move(args)));
    return;
  }

  std::unique_ptr<extensions::Event> event(
      new extensions::Event(histogram_value, event_name, std::move(args)));
  extensions::EventRouter::Get(browser_context_)
      ->BroadcastEvent(std::move(event));
}

net::NetLog* AtomExtensionsBrowserClient::GetNetLog() {
  return NULL;
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
  // TODO(samuelmaddock):
  return extensions::AtomExtensionWebContentsObserver::FromWebContents(
      web_contents);
}

extensions::ExtensionNavigationUIData*
AtomExtensionsBrowserClient::GetExtensionNavigationUIData(
    net::URLRequest* request) {
  content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(request);
  if (!info)
    return nullptr;
  extensions::AtomNavigationUIData* navigation_data =
      static_cast<extensions::AtomNavigationUIData*>(
          info->GetNavigationUIData());
  if (!navigation_data)
    return nullptr;
  return navigation_data->GetExtensionNavigationUIData();
}

extensions::KioskDelegate* AtomExtensionsBrowserClient::GetKioskDelegate() {
  return nullptr;
}

bool AtomExtensionsBrowserClient::IsLockScreenContext(
    content::BrowserContext* context) {
  return false;
}

std::string AtomExtensionsBrowserClient::GetApplicationLocale() {
  // TODO(michaelpg): Use system locale.
  return "en-US";
}

std::string AtomExtensionsBrowserClient::GetUserAgent() const {
  return content::BuildUserAgentFromProduct(
      version_info::GetProductNameAndVersionForUserAgent());
}

void AtomExtensionsBrowserClient::InitWithBrowserContext(
    content::BrowserContext* context,
    PrefService* pref_service) {
  DCHECK(!browser_context_);
  DCHECK(!pref_service_);
  browser_context_ = context;
  pref_service_ = pref_service;
}

}  // namespace atom
