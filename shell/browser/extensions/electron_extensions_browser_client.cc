// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extensions_browser_client.h"

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
#include "extensions/browser/mojo/interface_registration.h"
#include "extensions/browser/null_app_sorting.h"
#include "extensions/browser/updater/null_extension_cache.h"
#include "extensions/browser/url_request_util.h"
#include "extensions/common/features/feature_channel.h"
#include "shell/browser/browser.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/extensions/api/runtime/electron_runtime_api_delegate.h"
#include "shell/browser/extensions/electron_extension_host_delegate.h"
#include "shell/browser/extensions/electron_extension_system_factory.h"
#include "shell/browser/extensions/electron_extension_web_contents_observer.h"
// #include "shell/browser/extensions/atom_extensions_api_client.h"
// #include "shell/browser/extensions/atom_extensions_browser_api_provider.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "shell/browser/extensions/electron_navigation_ui_data.h"

using content::BrowserContext;
using content::BrowserThread;

namespace electron {

ElectronExtensionsBrowserClient::ElectronExtensionsBrowserClient()
    : api_client_(new extensions::ExtensionsAPIClient),
      // : api_client_(new extensions::ElectronExtensionsAPIClient),
      extension_cache_(new extensions::NullExtensionCache()) {
  // app_shell does not have a concept of channel yet, so leave UNKNOWN to
  // enable all channel-dependent extension APIs.
  extensions::SetCurrentChannel(version_info::Channel::UNKNOWN);

  AddAPIProvider(
      std::make_unique<extensions::CoreExtensionsBrowserAPIProvider>());
  // AddAPIProvider(std::make_unique<ElectronExtensionsBrowserAPIProvider>());
}

ElectronExtensionsBrowserClient::~ElectronExtensionsBrowserClient() {}

bool ElectronExtensionsBrowserClient::IsShuttingDown() {
  return electron::Browser::Get()->is_shutting_down();
}

bool ElectronExtensionsBrowserClient::AreExtensionsDisabled(
    const base::CommandLine& command_line,
    BrowserContext* context) {
  return false;
}

bool ElectronExtensionsBrowserClient::IsValidContext(BrowserContext* context) {
  auto context_map = ElectronBrowserContext::browser_context_map();
  for (auto const& entry : context_map) {
    if (entry.second && entry.second.get() == context)
      return true;
  }
  return false;
}

bool ElectronExtensionsBrowserClient::IsSameContext(BrowserContext* first,
                                                    BrowserContext* second) {
  return first == second;
}

bool ElectronExtensionsBrowserClient::HasOffTheRecordContext(
    BrowserContext* context) {
  return false;
}

BrowserContext* ElectronExtensionsBrowserClient::GetOffTheRecordContext(
    BrowserContext* context) {
  // app_shell only supports a single context.
  return nullptr;
}

BrowserContext* ElectronExtensionsBrowserClient::GetOriginalContext(
    BrowserContext* context) {
  DCHECK(context);
  if (context->IsOffTheRecord()) {
    return ElectronBrowserContext::From("", false).get();
  } else {
    return context;
  }
}

bool ElectronExtensionsBrowserClient::IsGuestSession(
    BrowserContext* context) const {
  return false;
}

bool ElectronExtensionsBrowserClient::IsExtensionIncognitoEnabled(
    const std::string& extension_id,
    content::BrowserContext* context) const {
  return false;
}

bool ElectronExtensionsBrowserClient::CanExtensionCrossIncognito(
    const extensions::Extension* extension,
    content::BrowserContext* context) const {
  return false;
}

base::FilePath ElectronExtensionsBrowserClient::GetBundleResourcePath(
    const network::ResourceRequest& request,
    const base::FilePath& extension_resources_path,
    int* resource_id) const {
  *resource_id = 0;
  return base::FilePath();
}

void ElectronExtensionsBrowserClient::LoadResourceFromResourceBundle(
    const network::ResourceRequest& request,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    const base::FilePath& resource_relative_path,
    int resource_id,
    const std::string& content_security_policy,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    bool send_cors_header) {
  NOTREACHED() << "Load resources from bundles not supported.";
}

bool ElectronExtensionsBrowserClient::AllowCrossRendererResourceLoad(
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

PrefService* ElectronExtensionsBrowserClient::GetPrefServiceForContext(
    BrowserContext* context) {
  return static_cast<ElectronBrowserContext*>(context)->prefs();
}

void ElectronExtensionsBrowserClient::GetEarlyExtensionPrefsObservers(
    content::BrowserContext* context,
    std::vector<extensions::EarlyExtensionPrefsObserver*>* observers) const {}

extensions::ProcessManagerDelegate*
ElectronExtensionsBrowserClient::GetProcessManagerDelegate() const {
  return NULL;
}

std::unique_ptr<extensions::ExtensionHostDelegate>
ElectronExtensionsBrowserClient::
    CreateExtensionHostDelegate() {  // TODO(samuelmaddock):
  return base::WrapUnique(new extensions::ElectronExtensionHostDelegate);
}

bool ElectronExtensionsBrowserClient::DidVersionUpdate(
    BrowserContext* context) {
  // TODO(jamescook): We might want to tell extensions when app_shell updates.
  return false;
}

void ElectronExtensionsBrowserClient::PermitExternalProtocolHandler() {}

bool ElectronExtensionsBrowserClient::IsInDemoMode() {
  return false;
}

bool ElectronExtensionsBrowserClient::IsScreensaverInDemoMode(
    const std::string& app_id) {
  return false;
}

bool ElectronExtensionsBrowserClient::IsRunningInForcedAppMode() {
  return false;
}

bool ElectronExtensionsBrowserClient::IsAppModeForcedForApp(
    const extensions::ExtensionId& extension_id) {
  return false;
}

bool ElectronExtensionsBrowserClient::IsLoggedInAsPublicAccount() {
  return false;
}

extensions::ExtensionSystemProvider*
ElectronExtensionsBrowserClient::GetExtensionSystemFactory() {
  return extensions::ElectronExtensionSystemFactory::GetInstance();
}

void ElectronExtensionsBrowserClient::RegisterExtensionInterfaces(
    service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>*
        registry,
    content::RenderFrameHost* render_frame_host,
    const extensions::Extension* extension) const {
  RegisterInterfacesForExtension(registry, render_frame_host, extension);
}

std::unique_ptr<extensions::RuntimeAPIDelegate>
ElectronExtensionsBrowserClient::CreateRuntimeAPIDelegate(
    content::BrowserContext* context) const {
  return std::make_unique<extensions::ElectronRuntimeAPIDelegate>(context);
}

const extensions::ComponentExtensionResourceManager*
ElectronExtensionsBrowserClient::GetComponentExtensionResourceManager() {
  return NULL;
}

void ElectronExtensionsBrowserClient::BroadcastEventToRenderers(
    extensions::events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::ListValue> args) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    base::PostTask(
        FROM_HERE, {BrowserThread::UI},
        base::BindOnce(
            &ElectronExtensionsBrowserClient::BroadcastEventToRenderers,
            base::Unretained(this), histogram_value, event_name,
            std::move(args)));
    return;
  }

  std::unique_ptr<extensions::Event> event(
      new extensions::Event(histogram_value, event_name, std::move(args)));
  auto context_map = ElectronBrowserContext::browser_context_map();
  for (auto const& entry : context_map) {
    if (entry.second) {
      extensions::EventRouter::Get(entry.second.get())
          ->BroadcastEvent(std::move(event));
    }
  }
}

extensions::ExtensionCache*
ElectronExtensionsBrowserClient::GetExtensionCache() {
  return extension_cache_.get();
}

bool ElectronExtensionsBrowserClient::IsBackgroundUpdateAllowed() {
  return true;
}

bool ElectronExtensionsBrowserClient::IsMinBrowserVersionSupported(
    const std::string& min_version) {
  return true;
}

void ElectronExtensionsBrowserClient::SetAPIClientForTest(
    extensions::ExtensionsAPIClient* api_client) {
  api_client_.reset(api_client);
}

extensions::ExtensionWebContentsObserver*
ElectronExtensionsBrowserClient::GetExtensionWebContentsObserver(
    content::WebContents* web_contents) {
  return extensions::ElectronExtensionWebContentsObserver::FromWebContents(
      web_contents);
}

extensions::KioskDelegate* ElectronExtensionsBrowserClient::GetKioskDelegate() {
  return nullptr;
}

bool ElectronExtensionsBrowserClient::IsLockScreenContext(
    content::BrowserContext* context) {
  return false;
}

std::string ElectronExtensionsBrowserClient::GetApplicationLocale() {
  return ElectronBrowserClient::Get()->GetApplicationLocale();
}

std::string ElectronExtensionsBrowserClient::GetUserAgent() const {
  return ElectronBrowserClient::Get()->GetUserAgent();
}

void ElectronExtensionsBrowserClient::RegisterBrowserInterfaceBindersForFrame(
    service_manager::BinderMapWithContext<content::RenderFrameHost*>* map,
    content::RenderFrameHost* render_frame_host,
    const extensions::Extension* extension) const {}

}  // namespace electron
