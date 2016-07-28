// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extensions_browser_client.h"

#include <string>
#include <vector>
#include "atom/browser/browser.h"
#include "atom/browser/extensions/api/atom_extensions_api_client.h"
#include "atom/browser/extensions/atom_extension_api_frame_id_map_helper.h"
#include "atom/browser/extensions/atom_extension_host_delegate.h"
#include "atom/browser/extensions/atom_extension_system_factory.h"
#include "atom/browser/extensions/atom_extension_web_contents_observer.h"
#include "atom/browser/extensions/atom_process_manager_delegate.h"
#include "atom/browser/web_contents_preferences.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/version.h"
#include "brave/browser/brave_browser_context.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_paths.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/api/alarms/alarms_api.h"
#include "extensions/browser/api/audio/audio_api.h"
#include "extensions/browser/api/declarative/declarative_api.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/api/idle/idle_api.h"
#include "extensions/browser/api/management/management_api.h"
#include "extensions/browser/api/runtime/runtime_api.h"
#include "extensions/browser/api/runtime/runtime_api_delegate.h"
#include "extensions/browser/api/socket/socket_api.h"
#include "extensions/browser/api/sockets_tcp/sockets_tcp_api.h"
#include "extensions/browser/api/sockets_tcp_server/sockets_tcp_server_api.h"
#include "extensions/browser/api/sockets_udp/sockets_udp_api.h"
#include "extensions/browser/api/storage/storage_api.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "extensions/browser/component_extension_resource_manager.h"
#include "extensions/browser/extension_function_registry.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/pref_names.h"
#include "extensions/browser/updater/null_extension_cache.h"
#include "extensions/browser/url_request_util.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "ui/base/resource/resource_bundle.h"

// URLRequestResourceBundleJob
// TODO(bridiver) move to a separate file
#include "base/base64.h"
#include "base/format_macros.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "extensions/common/file_util.h"
#include "net/url_request/url_request_simple_job.h"

namespace {

net::HttpResponseHeaders* BuildHttpHeaders(
    const std::string& content_security_policy,
    bool send_cors_header,
    const base::Time& last_modified_time) {
  std::string raw_headers;
  raw_headers.append("HTTP/1.1 200 OK");
  if (!content_security_policy.empty()) {
    raw_headers.append(1, '\0');
    raw_headers.append("Content-Security-Policy: ");
    raw_headers.append(content_security_policy);
  }

  if (send_cors_header) {
    raw_headers.append(1, '\0');
    raw_headers.append("Access-Control-Allow-Origin: *");
  }

  if (!last_modified_time.is_null()) {
    // Hash the time and make an etag to avoid exposing the exact
    // user installation time of the extension.
    std::string hash =
        base::StringPrintf("%" PRId64, last_modified_time.ToInternalValue());
    hash = base::SHA1HashString(hash);
    std::string etag;
    base::Base64Encode(hash, &etag);
    raw_headers.append(1, '\0');
    raw_headers.append("ETag: \"");
    raw_headers.append(etag);
    raw_headers.append("\"");
    // Also force revalidation.
    raw_headers.append(1, '\0');
    raw_headers.append("cache-control: no-cache");
  }

  raw_headers.append(2, '\0');
  return new net::HttpResponseHeaders(raw_headers);
}

// A request for an extension resource in a Chrome .pak file. These are used
// by component extensions.
class URLRequestResourceBundleJob : public net::URLRequestSimpleJob {
 public:
  URLRequestResourceBundleJob(net::URLRequest* request,
                              net::NetworkDelegate* network_delegate,
                              const base::FilePath& filename,
                              int resource_id,
                              const std::string& content_security_policy,
                              bool send_cors_header)
      : net::URLRequestSimpleJob(request, network_delegate),
        filename_(filename),
        resource_id_(resource_id),
        weak_factory_(this) {
    // Leave cache headers out of resource bundle requests.
    response_info_.headers = BuildHttpHeaders(
        content_security_policy, send_cors_header, base::Time());
  }

  // Overridden from URLRequestSimpleJob:
  int GetRefCountedData(
      std::string* mime_type,
      std::string* charset,
      scoped_refptr<base::RefCountedMemory>* data,
      const net::CompletionCallback& callback) const override {
    const ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    *data = rb.LoadDataResourceBytes(resource_id_);

    // Add the Content-Length header now that we know the resource length.
    response_info_.headers->AddHeader(
        base::StringPrintf("%s: %s", net::HttpRequestHeaders::kContentLength,
                           base::SizeTToString((*data)->size()).c_str()));

    std::string* read_mime_type = new std::string;
    bool posted = base::PostTaskAndReplyWithResult(
        content::BrowserThread::GetBlockingPool(), FROM_HERE,
        base::Bind(&net::GetMimeTypeFromFile, filename_,
                   base::Unretained(read_mime_type)),
        base::Bind(&URLRequestResourceBundleJob::OnMimeTypeRead,
                   weak_factory_.GetWeakPtr(), mime_type, charset, *data,
                   base::Owned(read_mime_type), callback));
    DCHECK(posted);

    return net::ERR_IO_PENDING;
  }

  void GetResponseInfo(net::HttpResponseInfo* info) override {
    *info = response_info_;
  }

 private:
  ~URLRequestResourceBundleJob() override {}

  void OnMimeTypeRead(std::string* out_mime_type,
                      std::string* charset,
                      scoped_refptr<base::RefCountedMemory> data,
                      std::string* read_mime_type,
                      const net::CompletionCallback& callback,
                      bool read_result) {
    *out_mime_type = *read_mime_type;
    if (base::StartsWith(*read_mime_type, "text/",
                         base::CompareCase::INSENSITIVE_ASCII)) {
      // All of our HTML files should be UTF-8 and for other resource types
      // (like images), charset doesn't matter.
      DCHECK(base::IsStringUTF8(base::StringPiece(
          reinterpret_cast<const char*>(data->front()), data->size())));
      *charset = "utf-8";
    }
    int result = read_result ? net::OK : net::ERR_INVALID_URL;
    callback.Run(result);
  }

  // We need the filename of the resource to determine the mime type.
  base::FilePath filename_;

  // The resource bundle id to load.
  int resource_id_;

  net::HttpResponseInfo response_info_;

  mutable base::WeakPtrFactory<URLRequestResourceBundleJob> weak_factory_;
};

}  // namespace

namespace extensions {

class AtomRuntimeAPIDelegate : public RuntimeAPIDelegate {
 public:
  AtomRuntimeAPIDelegate() {}
  ~AtomRuntimeAPIDelegate() override {};

  // RuntimeAPIDelegate implementation.
  void AddUpdateObserver(UpdateObserver* observer) override {};
  void RemoveUpdateObserver(UpdateObserver* observer) override {};
  base::Version GetPreviousExtensionVersion(
      const Extension* extension) override {
    return base::Version();
  };
  void ReloadExtension(const std::string& extension_id) override {};
  bool CheckForUpdates(const std::string& extension_id,
                       const UpdateCheckCallback& callback) override {
    return false;
  };
  void OpenURL(const GURL& uninstall_url) override {};
  bool GetPlatformInfo(api::runtime::PlatformInfo* info) override {
    return true;
  };
  bool RestartDevice(std::string* error_message) override { return false; };

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomRuntimeAPIDelegate);
};

class AtomComponentExtensionResourceManager
    : public ComponentExtensionResourceManager {
 public:
  AtomComponentExtensionResourceManager() {}
  ~AtomComponentExtensionResourceManager() override {};

  bool IsComponentExtensionResource(
      const base::FilePath& extension_path,
      const base::FilePath& resource_path,
      int* resource_id) const override {
    // TODO(bridiver) - bundle extension resources
    return false;
  };

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomComponentExtensionResourceManager);
};

AtomExtensionsBrowserClient::AtomExtensionsBrowserClient()
  : extension_cache_(new NullExtensionCache()) {
    api_client_.reset(new AtomExtensionsAPIClient);
    process_manager_delegate_.reset(new AtomProcessManagerDelegate);
    resource_manager_.reset(new AtomComponentExtensionResourceManager);
}

AtomExtensionsBrowserClient::~AtomExtensionsBrowserClient() {}

std::unique_ptr<ExtensionApiFrameIdMapHelper>
AtomExtensionsBrowserClient::CreateExtensionApiFrameIdMapHelper(
    ExtensionApiFrameIdMap* map) {
  return base::WrapUnique(new AtomExtensionApiFrameIdMapHelper(map));
}

bool AtomExtensionsBrowserClient::IsShuttingDown() {
  return atom::Browser::Get()->is_shutting_down();
}

bool AtomExtensionsBrowserClient::AreExtensionsDisabled(
    const base::CommandLine& command_line,
    content::BrowserContext* context) {
  return false;
}

bool AtomExtensionsBrowserClient::IsValidContext(
    content::BrowserContext* context) {
  return true;
}

bool AtomExtensionsBrowserClient::IsSameContext(
    content::BrowserContext* first,
    content::BrowserContext* second) {
  if (GetOriginalContext(first) == GetOriginalContext(second))
    return true;

  return false;
}

bool AtomExtensionsBrowserClient::HasOffTheRecordContext(
    content::BrowserContext* context) {
  return static_cast<brave::BraveBrowserContext*>(context)->otr_context()
      != nullptr;
}

content::BrowserContext* AtomExtensionsBrowserClient::GetOffTheRecordContext(
    content::BrowserContext* context) {
  return static_cast<brave::BraveBrowserContext*>(context)->otr_context();
}

content::BrowserContext* AtomExtensionsBrowserClient::GetOriginalContext(
    content::BrowserContext* context) {
  return static_cast<brave::BraveBrowserContext*>(context)->original_context();
}

bool AtomExtensionsBrowserClient::IsGuestSession(
    content::BrowserContext* context) const {
  return false;
}

// static
bool AtomExtensionsBrowserClient::IsIncognitoEnabled(
      const std::string& extension_id,
      content::BrowserContext* context) {
  auto original_context =
      static_cast<brave::BraveBrowserContext*>(context)->original_context();

  auto registry = ExtensionRegistry::Get(original_context);
  if (!registry)
    return false;

  const Extension* extension = registry->
      GetExtensionById(extension_id, ExtensionRegistry::ENABLED);
  if (extension) {
    if (!util::CanBeIncognitoEnabled(extension))
      return false;
    if (extension->location() == Manifest::COMPONENT)
      return true;
  }

  return true;
}

bool AtomExtensionsBrowserClient::CanExtensionCrossIncognito(
    const Extension* extension,
    content::BrowserContext* context) const {
  DCHECK(extension);
  return IsExtensionIncognitoEnabled(extension->id(), context) &&
         !IncognitoInfo::IsSplitMode(extension);
}

net::URLRequestJob*
AtomExtensionsBrowserClient::MaybeCreateResourceBundleRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const base::FilePath& directory_path,
    const std::string& content_security_policy,
    bool send_cors_header) {
  base::FilePath resources_path;
  base::FilePath relative_path;

  PathService::Get(chrome::DIR_RESOURCES, &resources_path);

  if (PathService::Get(chrome::DIR_RESOURCES, &resources_path) &&
      resources_path.AppendRelativePath(directory_path, &relative_path)) {
    base::FilePath request_path =
        extensions::file_util::ExtensionURLToRelativeFilePath(request->url());
    int resource_id = 0;
    if (ExtensionsBrowserClient::Get()
            ->GetComponentExtensionResourceManager()
            ->IsComponentExtensionResource(
                directory_path, request_path, &resource_id)) {
      relative_path = relative_path.Append(request_path);
      relative_path = relative_path.NormalizePathSeparators();

      return new URLRequestResourceBundleJob(request,
                                             network_delegate,
                                             relative_path,
                                             resource_id,
                                             content_security_policy,
                                             send_cors_header);
    }
  }
  return NULL;
}

bool AtomExtensionsBrowserClient::AllowCrossRendererResourceLoad(
    net::URLRequest* request,
    bool is_incognito,
    const Extension* extension,
    InfoMap* extension_info_map) {
  bool allowed = false;
  if (url_request_util::AllowCrossRendererResourceLoad(
          request, is_incognito, extension, extension_info_map, &allowed)) {
    return allowed;
  }

  // Couldn't determine if resource is allowed. Block the load.
  return false;
}

void AtomExtensionsBrowserClient::GetEarlyExtensionPrefsObservers(
    content::BrowserContext* context,
    std::vector<ExtensionPrefsObserver*>* observers) const {
}

ProcessManagerDelegate*
AtomExtensionsBrowserClient::GetProcessManagerDelegate() const {
  return process_manager_delegate_.get();
}

std::unique_ptr<ExtensionHostDelegate>
AtomExtensionsBrowserClient::CreateExtensionHostDelegate() {
  return std::unique_ptr<ExtensionHostDelegate>(new AtomExtensionHostDelegate);
}

PrefService* AtomExtensionsBrowserClient::GetPrefServiceForContext(
    content::BrowserContext* context) {
  return user_prefs::UserPrefs::Get(context);
}

bool AtomExtensionsBrowserClient::DidVersionUpdate(
    content::BrowserContext* context) {
  return false;
}

void AtomExtensionsBrowserClient::PermitExternalProtocolHandler() {
}

bool AtomExtensionsBrowserClient::IsRunningInForcedAppMode() {
  return false;
}

bool AtomExtensionsBrowserClient::IsLoggedInAsPublicAccount() {
  return false;
}

ApiActivityMonitor* AtomExtensionsBrowserClient::GetApiActivityMonitor(
    content::BrowserContext* context) {
  // The ActivityLog monitors and records function calls and events.
  return nullptr;
}

ExtensionSystemProvider*
AtomExtensionsBrowserClient::GetExtensionSystemFactory() {
  return AtomExtensionSystemFactory::GetInstance();
}

void AtomExtensionsBrowserClient::RegisterExtensionFunctions(
    ExtensionFunctionRegistry* registry) const {
  registry->RegisterFunction<WebRequestHandlerBehaviorChangedFunction>();
  registry->RegisterFunction<api::SocketsUdpCreateFunction>();
  registry->RegisterFunction<api::SocketsUdpUpdateFunction>();
  registry->RegisterFunction<api::SocketsUdpSetPausedFunction>();
  registry->RegisterFunction<api::SocketsUdpBindFunction>();
  registry->RegisterFunction<api::SocketsUdpSendFunction>();
  registry->RegisterFunction<api::SocketsUdpCloseFunction>();
  registry->RegisterFunction<api::SocketsUdpGetInfoFunction>();
  registry->RegisterFunction<api::SocketsUdpGetSocketsFunction>();
  registry->RegisterFunction<api::SocketsUdpJoinGroupFunction>();
  registry->RegisterFunction<api::SocketsUdpLeaveGroupFunction>();
  registry->RegisterFunction<api::SocketsUdpSetMulticastTimeToLiveFunction>();
  registry->RegisterFunction<api::SocketsUdpSetMulticastLoopbackModeFunction>();
  registry->RegisterFunction<api::SocketsUdpGetJoinedGroupsFunction>();
  registry->RegisterFunction<api::SocketsUdpSetBroadcastFunction>();
  registry->RegisterFunction<StorageStorageAreaGetFunction>();
  registry->RegisterFunction<StorageStorageAreaGetBytesInUseFunction>();
  registry->RegisterFunction<StorageStorageAreaSetFunction>();
  registry->RegisterFunction<StorageStorageAreaRemoveFunction>();
  registry->RegisterFunction<StorageStorageAreaClearFunction>();
  registry->RegisterFunction<EventsEventAddRulesFunction>();
  registry->RegisterFunction<EventsEventGetRulesFunction>();
  registry->RegisterFunction<EventsEventRemoveRulesFunction>();
  registry->RegisterFunction<AudioGetInfoFunction>();
  registry->RegisterFunction<AudioSetActiveDevicesFunction>();
  registry->RegisterFunction<AudioSetPropertiesFunction>();
  registry->RegisterFunction<SocketCreateFunction>();
  registry->RegisterFunction<SocketDestroyFunction>();
  registry->RegisterFunction<SocketConnectFunction>();
  registry->RegisterFunction<SocketBindFunction>();
  registry->RegisterFunction<SocketDisconnectFunction>();
  registry->RegisterFunction<SocketReadFunction>();
  registry->RegisterFunction<SocketWriteFunction>();
  registry->RegisterFunction<SocketRecvFromFunction>();
  registry->RegisterFunction<SocketSendToFunction>();
  registry->RegisterFunction<SocketListenFunction>();
  registry->RegisterFunction<SocketAcceptFunction>();
  registry->RegisterFunction<SocketSetKeepAliveFunction>();
  registry->RegisterFunction<SocketSetNoDelayFunction>();
  registry->RegisterFunction<SocketGetInfoFunction>();
  registry->RegisterFunction<SocketGetNetworkListFunction>();
  registry->RegisterFunction<SocketJoinGroupFunction>();
  registry->RegisterFunction<SocketLeaveGroupFunction>();
  registry->RegisterFunction<SocketSetMulticastTimeToLiveFunction>();
  registry->RegisterFunction<SocketSetMulticastLoopbackModeFunction>();
  registry->RegisterFunction<SocketGetJoinedGroupsFunction>();
  registry->RegisterFunction<SocketSecureFunction>();
  registry->RegisterFunction<api::SocketsTcpCreateFunction>();
  registry->RegisterFunction<api::SocketsTcpUpdateFunction>();
  registry->RegisterFunction<api::SocketsTcpSetPausedFunction>();
  registry->RegisterFunction<api::SocketsTcpSetKeepAliveFunction>();
  registry->RegisterFunction<api::SocketsTcpSetNoDelayFunction>();
  registry->RegisterFunction<api::SocketsTcpConnectFunction>();
  registry->RegisterFunction<api::SocketsTcpDisconnectFunction>();
  registry->RegisterFunction<api::SocketsTcpSecureFunction>();
  registry->RegisterFunction<api::SocketsTcpSendFunction>();
  registry->RegisterFunction<api::SocketsTcpCloseFunction>();
  registry->RegisterFunction<api::SocketsTcpGetInfoFunction>();
  registry->RegisterFunction<api::SocketsTcpGetSocketsFunction>();
  registry->RegisterFunction<IdleQueryStateFunction>();
  registry->RegisterFunction<IdleSetDetectionIntervalFunction>();
  registry->RegisterFunction<api::SocketsTcpServerCreateFunction>();
  registry->RegisterFunction<api::SocketsTcpServerUpdateFunction>();
  registry->RegisterFunction<api::SocketsTcpServerSetPausedFunction>();
  registry->RegisterFunction<api::SocketsTcpServerListenFunction>();
  registry->RegisterFunction<api::SocketsTcpServerDisconnectFunction>();
  registry->RegisterFunction<api::SocketsTcpServerCloseFunction>();
  registry->RegisterFunction<api::SocketsTcpServerGetInfoFunction>();
  registry->RegisterFunction<api::SocketsTcpServerGetSocketsFunction>();
  registry->RegisterFunction<AlarmsCreateFunction>();
  registry->RegisterFunction<AlarmsGetFunction>();
  registry->RegisterFunction<AlarmsGetAllFunction>();
  registry->RegisterFunction<AlarmsClearFunction>();
  registry->RegisterFunction<AlarmsClearAllFunction>();
  registry->RegisterFunction<ManagementGetAllFunction>();
  registry->RegisterFunction<ManagementGetFunction>();
  registry->RegisterFunction<ManagementGetSelfFunction>();
  registry->RegisterFunction<ManagementGetPermissionWarningsByIdFunction>();
  registry->RegisterFunction<
      ManagementGetPermissionWarningsByManifestFunction>();
  registry->RegisterFunction<ManagementSetEnabledFunction>();
  registry->RegisterFunction<ManagementUninstallFunction>();
  registry->RegisterFunction<ManagementUninstallSelfFunction>();
  registry->RegisterFunction<ManagementLaunchAppFunction>();
  registry->RegisterFunction<ManagementCreateAppShortcutFunction>();
  registry->RegisterFunction<ManagementSetLaunchTypeFunction>();
  registry->RegisterFunction<ManagementGenerateAppForLinkFunction>();
  registry->RegisterFunction<RuntimeGetBackgroundPageFunction>();
  registry->RegisterFunction<RuntimeOpenOptionsPageFunction>();
  registry->RegisterFunction<RuntimeSetUninstallURLFunction>();
  registry->RegisterFunction<RuntimeReloadFunction>();
  registry->RegisterFunction<RuntimeRequestUpdateCheckFunction>();
  registry->RegisterFunction<RuntimeRestartFunction>();
  registry->RegisterFunction<RuntimeGetPlatformInfoFunction>();
  registry->RegisterFunction<RuntimeGetPackageDirectoryEntryFunction>();
  registry->RegisterFunction<WebRequestInternalAddEventListenerFunction>();
  registry->RegisterFunction<WebRequestInternalEventHandledFunction>();
}

std::unique_ptr<RuntimeAPIDelegate>
AtomExtensionsBrowserClient::CreateRuntimeAPIDelegate(
    content::BrowserContext* context) const {
  return std::unique_ptr<RuntimeAPIDelegate>(new AtomRuntimeAPIDelegate());
}

const ComponentExtensionResourceManager*
AtomExtensionsBrowserClient::GetComponentExtensionResourceManager() {
  return resource_manager_.get();
}

void AtomExtensionsBrowserClient::RegisterMojoServices(
    content::RenderFrameHost* render_frame_host,
    const Extension* extension) const {
}

void AtomExtensionsBrowserClient::BroadcastEventToRenderers(
    events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::ListValue> args) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::Bind(&AtomExtensionsBrowserClient::BroadcastEventToRenderers,
                   base::Unretained(this), histogram_value, event_name,
                   base::Passed(&args)));
    return;
  }

  std::unique_ptr<Event> event(
      new Event(histogram_value, event_name, std::move(args)));
  EventRouter::Get(browser_context_)->BroadcastEvent(std::move(event));
}

net::NetLog* AtomExtensionsBrowserClient::GetNetLog() {
  return nullptr;
}

ExtensionCache* AtomExtensionsBrowserClient::GetExtensionCache() {
  return extension_cache_.get();
}

bool AtomExtensionsBrowserClient::IsBackgroundUpdateAllowed() {
  return true;
}

bool AtomExtensionsBrowserClient::IsMinBrowserVersionSupported(
    const std::string& min_version) {
  return true;
}

ExtensionWebContentsObserver*
AtomExtensionsBrowserClient::GetExtensionWebContentsObserver(
    content::WebContents* web_contents) {
  return AtomExtensionWebContentsObserver::FromWebContents(web_contents);;
}

void AtomExtensionsBrowserClient::CleanUpWebView(
    content::BrowserContext* browser_context,
    int embedder_process_id,
    int view_instance_id) {
}

void AtomExtensionsBrowserClient::AttachExtensionTaskManagerTag(
    content::WebContents* web_contents,
    ViewType view_type) {
}

}  // namespace extensions
