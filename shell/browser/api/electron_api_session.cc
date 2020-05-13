// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_session.h"

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/pref_names.h"
#include "components/download/public/common/download_danger_type.h"
#include "components/download/public/common/download_url_parameters.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/value_map_pref_store.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/download_manager_delegate.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/storage_partition.h"
#include "gin/arguments.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/completion_repeating_callback.h"
#include "net/base/load_flags.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_cache.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/features.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/browser/api/electron_api_cookies.h"
#include "shell/browser/api/electron_api_data_pipe_holder.h"
#include "shell/browser/api/electron_api_download_item.h"
#include "shell/browser/api/electron_api_net_log.h"
#include "shell/browser/api/electron_api_protocol.h"
#include "shell/browser/api/electron_api_service_worker_context.h"
#include "shell/browser/api/electron_api_web_request.h"
#include "shell/browser/browser.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/electron_permission_manager.h"
#include "shell/browser/media/media_device_id_salt.h"
#include "shell/browser/net/cert_verifier_client.h"
#include "shell/browser/session_preferences.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "shell/common/process_util.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#include "shell/browser/extensions/electron_extension_system.h"
#include "shell/common/gin_converters/extension_converter.h"
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "chrome/browser/spellchecker/spellcheck_factory.h"  // nogncheck
#include "chrome/browser/spellchecker/spellcheck_service.h"  // nogncheck
#include "components/spellcheck/browser/pref_names.h"
#include "components/spellcheck/common/spellcheck_common.h"

#if BUILDFLAG(USE_BROWSER_SPELLCHECKER)
#include "components/spellcheck/browser/spellcheck_platform.h"
#include "components/spellcheck/common/spellcheck_features.h"
#endif
#endif

using content::BrowserThread;
using content::StoragePartition;

namespace predictors {
// NOTE(nornagon): this is copied from
// //chrome/browser/predictors/resource_prefetch_predictor.cc we don't need
// anything in that file other than this constructor. Without it we get a link
// error. Probably upstream the constructor should be moved to
// preconnect_manager.cc.
PreconnectRequest::PreconnectRequest(
    const url::Origin& origin,
    int num_sockets,
    const net::NetworkIsolationKey& network_isolation_key)
    : origin(origin),
      num_sockets(num_sockets),
      network_isolation_key(network_isolation_key) {
  DCHECK_GE(num_sockets, 0);
}
}  // namespace predictors

namespace {

struct ClearStorageDataOptions {
  GURL origin;
  uint32_t storage_types = StoragePartition::REMOVE_DATA_MASK_ALL;
  uint32_t quota_types = StoragePartition::QUOTA_MANAGED_STORAGE_MASK_ALL;
};

uint32_t GetStorageMask(const std::vector<std::string>& storage_types) {
  uint32_t storage_mask = 0;
  for (const auto& it : storage_types) {
    auto type = base::ToLowerASCII(it);
    if (type == "appcache")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_APPCACHE;
    else if (type == "cookies")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_COOKIES;
    else if (type == "filesystem")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_FILE_SYSTEMS;
    else if (type == "indexdb")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_INDEXEDDB;
    else if (type == "localstorage")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_LOCAL_STORAGE;
    else if (type == "shadercache")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_SHADER_CACHE;
    else if (type == "websql")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_WEBSQL;
    else if (type == "serviceworkers")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_SERVICE_WORKERS;
    else if (type == "cachestorage")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_CACHE_STORAGE;
  }
  return storage_mask;
}

uint32_t GetQuotaMask(const std::vector<std::string>& quota_types) {
  uint32_t quota_mask = 0;
  for (const auto& it : quota_types) {
    auto type = base::ToLowerASCII(it);
    if (type == "temporary")
      quota_mask |= StoragePartition::QUOTA_MANAGED_STORAGE_MASK_TEMPORARY;
    else if (type == "persistent")
      quota_mask |= StoragePartition::QUOTA_MANAGED_STORAGE_MASK_PERSISTENT;
    else if (type == "syncable")
      quota_mask |= StoragePartition::QUOTA_MANAGED_STORAGE_MASK_SYNCABLE;
  }
  return quota_mask;
}

}  // namespace

namespace gin {

template <>
struct Converter<ClearStorageDataOptions> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     ClearStorageDataOptions* out) {
    gin_helper::Dictionary options;
    if (!ConvertFromV8(isolate, val, &options))
      return false;
    options.Get("origin", &out->origin);
    std::vector<std::string> types;
    if (options.Get("storages", &types))
      out->storage_types = GetStorageMask(types);
    if (options.Get("quotas", &types))
      out->quota_types = GetQuotaMask(types);
    return true;
  }
};

}  // namespace gin

namespace electron {

namespace api {

namespace {

const char kPersistPrefix[] = "persist:";

void DownloadIdCallback(content::DownloadManager* download_manager,
                        const base::FilePath& path,
                        const std::vector<GURL>& url_chain,
                        const std::string& mime_type,
                        int64_t offset,
                        int64_t length,
                        const std::string& last_modified,
                        const std::string& etag,
                        const base::Time& start_time,
                        uint32_t id) {
  download_manager->CreateDownloadItem(
      base::GenerateGUID(), id, path, path, url_chain, GURL(), GURL(), GURL(),
      GURL(), base::nullopt, mime_type, mime_type, start_time, base::Time(),
      etag, last_modified, offset, length, std::string(),
      download::DownloadItem::INTERRUPTED,
      download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
      download::DOWNLOAD_INTERRUPT_REASON_NETWORK_TIMEOUT, false, base::Time(),
      false, std::vector<download::DownloadItem::ReceivedSlice>());
}

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
class DictionaryObserver final : public SpellcheckCustomDictionary::Observer {
 private:
  std::unique_ptr<gin_helper::Promise<std::set<std::string>>> promise_;
  base::WeakPtr<SpellcheckService> spellcheck_;

 public:
  DictionaryObserver(gin_helper::Promise<std::set<std::string>> promise,
                     base::WeakPtr<SpellcheckService> spellcheck)
      : spellcheck_(spellcheck) {
    promise_ = std::make_unique<gin_helper::Promise<std::set<std::string>>>(
        std::move(promise));
    if (spellcheck_)
      spellcheck_->GetCustomDictionary()->AddObserver(this);
  }

  ~DictionaryObserver() {
    if (spellcheck_)
      spellcheck_->GetCustomDictionary()->RemoveObserver(this);
  }

  void OnCustomDictionaryLoaded() override {
    if (spellcheck_) {
      promise_->Resolve(spellcheck_->GetCustomDictionary()->GetWords());
    } else {
      promise_->RejectWithErrorMessage(
          "Spellcheck in unexpected state: failed to load custom dictionary.");
    }
    delete this;
  }

  void OnCustomDictionaryChanged(
      const SpellcheckCustomDictionary::Change& dictionary_change) override {
    // noop
  }
};
#endif  // BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)

struct UserDataLink : base::SupportsUserData::Data {
  explicit UserDataLink(Session* ses) : session(ses) {}

  Session* session;
};

const void* kElectronApiSessionKey = &kElectronApiSessionKey;

}  // namespace

gin::WrapperInfo Session::kWrapperInfo = {gin::kEmbedderNativeGin};

Session::Session(v8::Isolate* isolate, ElectronBrowserContext* browser_context)
    : network_emulation_token_(base::UnguessableToken::Create()),
      browser_context_(browser_context) {
  // Observe DownloadManager to get download notifications.
  content::BrowserContext::GetDownloadManager(browser_context)
      ->AddObserver(this);

  new SessionPreferences(browser_context);

  protocol_.Reset(isolate, Protocol::Create(isolate, browser_context).ToV8());

  browser_context->SetUserData(kElectronApiSessionKey,
                               std::make_unique<UserDataLink>(this));

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  SpellcheckService* service =
      SpellcheckServiceFactory::GetForContext(browser_context_.get());
  if (service) {
    service->SetHunspellObserver(this);
  }
#endif
}

Session::~Session() {
  content::BrowserContext::GetDownloadManager(browser_context())
      ->RemoveObserver(this);

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  SpellcheckService* service =
      SpellcheckServiceFactory::GetForContext(browser_context_.get());
  if (service) {
    service->SetHunspellObserver(nullptr);
  }
#endif
}

void Session::OnDownloadCreated(content::DownloadManager* manager,
                                download::DownloadItem* item) {
  if (item->IsSavePackageDownload())
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  auto handle = DownloadItem::FromOrCreate(isolate, item);
  if (item->GetState() == download::DownloadItem::INTERRUPTED)
    handle->SetSavePath(item->GetTargetFilePath());
  content::WebContents* web_contents =
      content::DownloadItemUtils::GetWebContents(item);
  bool prevent_default = Emit("will-download", handle, web_contents);
  if (prevent_default) {
    item->Cancel(true);
    item->Remove();
  }
}

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
void Session::OnHunspellDictionaryInitialized(const std::string& language) {
  Emit("spellcheck-dictionary-initialized", language);
}
void Session::OnHunspellDictionaryDownloadBegin(const std::string& language) {
  Emit("spellcheck-dictionary-download-begin", language);
}
void Session::OnHunspellDictionaryDownloadSuccess(const std::string& language) {
  Emit("spellcheck-dictionary-download-success", language);
}
void Session::OnHunspellDictionaryDownloadFailure(const std::string& language) {
  Emit("spellcheck-dictionary-download-failure", language);
}
#endif

v8::Local<v8::Promise> Session::ResolveProxy(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<std::string> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  GURL url;
  args->GetNext(&url);

  browser_context_->GetResolveProxyHelper()->ResolveProxy(
      url, base::BindOnce(gin_helper::Promise<std::string>::ResolvePromise,
                          std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> Session::GetCacheSize() {
  auto* isolate = v8::Isolate::GetCurrent();
  gin_helper::Promise<int64_t> promise(isolate);
  auto handle = promise.GetHandle();

  content::BrowserContext::GetDefaultStoragePartition(browser_context_.get())
      ->GetNetworkContext()
      ->ComputeHttpCacheSize(
          base::Time(), base::Time::Max(),
          base::BindOnce(
              [](gin_helper::Promise<int64_t> promise, bool is_upper_bound,
                 int64_t size_or_error) {
                if (size_or_error < 0) {
                  promise.RejectWithErrorMessage(
                      net::ErrorToString(size_or_error));
                } else {
                  promise.Resolve(size_or_error);
                }
              },
              std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> Session::ClearCache() {
  auto* isolate = v8::Isolate::GetCurrent();
  gin_helper::Promise<void> promise(isolate);
  auto handle = promise.GetHandle();

  content::BrowserContext::GetDefaultStoragePartition(browser_context_.get())
      ->GetNetworkContext()
      ->ClearHttpCache(base::Time(), base::Time::Max(), nullptr,
                       base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                                      std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> Session::ClearStorageData(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  ClearStorageDataOptions options;
  args->GetNext(&options);

  auto* storage_partition =
      content::BrowserContext::GetStoragePartition(browser_context(), nullptr);
  if (options.storage_types & StoragePartition::REMOVE_DATA_MASK_COOKIES) {
    // Reset media device id salt when cookies are cleared.
    // https://w3c.github.io/mediacapture-main/#dom-mediadeviceinfo-deviceid
    MediaDeviceIDSalt::Reset(browser_context()->prefs());
  }

  storage_partition->ClearData(
      options.storage_types, options.quota_types, options.origin, base::Time(),
      base::Time::Max(),
      base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                     std::move(promise)));
  return handle;
}

void Session::FlushStorageData() {
  auto* storage_partition =
      content::BrowserContext::GetStoragePartition(browser_context(), nullptr);
  storage_partition->Flush();
}

v8::Local<v8::Promise> Session::SetProxy(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  gin_helper::Dictionary options;
  args->GetNext(&options);

  if (!browser_context_->in_memory_pref_store()) {
    promise.Resolve();
    return handle;
  }

  std::string proxy_rules, bypass_list, pac_url;

  options.Get("pacScript", &pac_url);
  options.Get("proxyRules", &proxy_rules);
  options.Get("proxyBypassRules", &bypass_list);

  // pacScript takes precedence over proxyRules.
  if (!pac_url.empty()) {
    browser_context_->in_memory_pref_store()->SetValue(
        proxy_config::prefs::kProxy,
        std::make_unique<base::Value>(ProxyConfigDictionary::CreatePacScript(
            pac_url, true /* pac_mandatory */)),
        WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  } else {
    browser_context_->in_memory_pref_store()->SetValue(
        proxy_config::prefs::kProxy,
        std::make_unique<base::Value>(ProxyConfigDictionary::CreateFixedServers(
            proxy_rules, bypass_list)),
        WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                                std::move(promise)));

  return handle;
}

void Session::SetDownloadPath(const base::FilePath& path) {
  browser_context_->prefs()->SetFilePath(prefs::kDownloadDefaultDirectory,
                                         path);
}

void Session::EnableNetworkEmulation(const gin_helper::Dictionary& options) {
  auto conditions = network::mojom::NetworkConditions::New();

  options.Get("offline", &conditions->offline);
  options.Get("downloadThroughput", &conditions->download_throughput);
  options.Get("uploadThroughput", &conditions->upload_throughput);
  double latency = 0.0;
  if (options.Get("latency", &latency) && latency) {
    conditions->latency = base::TimeDelta::FromMillisecondsD(latency);
  }

  auto* network_context = content::BrowserContext::GetDefaultStoragePartition(
                              browser_context_.get())
                              ->GetNetworkContext();
  network_context->SetNetworkConditions(network_emulation_token_,
                                        std::move(conditions));
}

void Session::DisableNetworkEmulation() {
  auto* network_context = content::BrowserContext::GetDefaultStoragePartition(
                              browser_context_.get())
                              ->GetNetworkContext();
  network_context->SetNetworkConditions(
      network_emulation_token_, network::mojom::NetworkConditions::New());
}

void Session::SetCertVerifyProc(v8::Local<v8::Value> val,
                                gin::Arguments* args) {
  CertVerifierClient::CertVerifyProc proc;
  if (!(val->IsNull() || gin::ConvertFromV8(args->isolate(), val, &proc))) {
    args->ThrowTypeError("Must pass null or function");
    return;
  }

  mojo::PendingRemote<network::mojom::CertVerifierClient>
      cert_verifier_client_remote;
  if (proc) {
    mojo::MakeSelfOwnedReceiver(
        std::make_unique<CertVerifierClient>(proc),
        cert_verifier_client_remote.InitWithNewPipeAndPassReceiver());
  }
  content::BrowserContext::GetDefaultStoragePartition(browser_context_.get())
      ->GetNetworkContext()
      ->SetCertVerifierClient(std::move(cert_verifier_client_remote));

  // This causes the cert verifier cache to be cleared.
  content::GetNetworkService()->OnCertDBChanged();
}

void Session::SetPermissionRequestHandler(v8::Local<v8::Value> val,
                                          gin::Arguments* args) {
  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context()->GetPermissionControllerDelegate());
  if (val->IsNull()) {
    permission_manager->SetPermissionRequestHandler(
        ElectronPermissionManager::RequestHandler());
    return;
  }
  auto handler = std::make_unique<ElectronPermissionManager::RequestHandler>();
  if (!gin::ConvertFromV8(args->isolate(), val, handler.get())) {
    args->ThrowTypeError("Must pass null or function");
    return;
  }
  permission_manager->SetPermissionRequestHandler(base::BindRepeating(
      [](ElectronPermissionManager::RequestHandler* handler,
         content::WebContents* web_contents,
         content::PermissionType permission_type,
         ElectronPermissionManager::StatusCallback callback,
         const base::Value& details) {
        handler->Run(web_contents, permission_type,
                     base::AdaptCallbackForRepeating(std::move(callback)),
                     details);
      },
      base::Owned(std::move(handler))));
}

void Session::SetPermissionCheckHandler(v8::Local<v8::Value> val,
                                        gin::Arguments* args) {
  ElectronPermissionManager::CheckHandler handler;
  if (!(val->IsNull() || gin::ConvertFromV8(args->isolate(), val, &handler))) {
    args->ThrowTypeError("Must pass null or function");
    return;
  }
  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context()->GetPermissionControllerDelegate());
  permission_manager->SetPermissionCheckHandler(handler);
}

v8::Local<v8::Promise> Session::ClearHostResolverCache(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  content::BrowserContext::GetDefaultStoragePartition(browser_context_.get())
      ->GetNetworkContext()
      ->ClearHostCache(nullptr,
                       base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                                      std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> Session::ClearAuthCache() {
  auto* isolate = v8::Isolate::GetCurrent();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  content::BrowserContext::GetDefaultStoragePartition(browser_context_.get())
      ->GetNetworkContext()
      ->ClearHttpAuthCache(
          base::Time(),
          base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                         std::move(promise)));

  return handle;
}

void Session::AllowNTLMCredentialsForDomains(const std::string& domains) {
  network::mojom::HttpAuthDynamicParamsPtr auth_dynamic_params =
      network::mojom::HttpAuthDynamicParams::New();
  auth_dynamic_params->server_allowlist = domains;
  auth_dynamic_params->enable_negotiate_port =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          electron::switches::kEnableAuthNegotiatePort);
  content::GetNetworkService()->ConfigureHttpAuthPrefs(
      std::move(auth_dynamic_params));
}

void Session::SetUserAgent(const std::string& user_agent,
                           gin::Arguments* args) {
  browser_context_->SetUserAgent(user_agent);
  content::BrowserContext::GetDefaultStoragePartition(browser_context_.get())
      ->GetNetworkContext()
      ->SetUserAgent(user_agent);
}

std::string Session::GetUserAgent() {
  return browser_context_->GetUserAgent();
}

bool Session::IsPersistent() {
  return !browser_context_->IsOffTheRecord();
}

v8::Local<v8::Promise> Session::GetBlobData(v8::Isolate* isolate,
                                            const std::string& uuid) {
  gin::Handle<DataPipeHolder> holder = DataPipeHolder::From(isolate, uuid);
  if (holder.IsEmpty()) {
    gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
    promise.RejectWithErrorMessage("Could not get blob data handle");
    return promise.GetHandle();
  }

  return holder->ReadAll(isolate);
}

void Session::DownloadURL(const GURL& url) {
  auto* download_manager =
      content::BrowserContext::GetDownloadManager(browser_context());
  auto download_params = std::make_unique<download::DownloadUrlParameters>(
      url, MISSING_TRAFFIC_ANNOTATION);
  download_manager->DownloadUrl(std::move(download_params));
}

void Session::CreateInterruptedDownload(const gin_helper::Dictionary& options) {
  int64_t offset = 0, length = 0;
  double start_time = base::Time::Now().ToDoubleT();
  std::string mime_type, last_modified, etag;
  base::FilePath path;
  std::vector<GURL> url_chain;
  options.Get("path", &path);
  options.Get("urlChain", &url_chain);
  options.Get("mimeType", &mime_type);
  options.Get("offset", &offset);
  options.Get("length", &length);
  options.Get("lastModified", &last_modified);
  options.Get("eTag", &etag);
  options.Get("startTime", &start_time);
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (path.empty() || url_chain.empty() || length == 0) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Must pass non-empty path, urlChain and length.")));
    return;
  }
  if (offset >= length) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate, "Must pass an offset value less than length.")));
    return;
  }
  auto* download_manager =
      content::BrowserContext::GetDownloadManager(browser_context());
  download_manager->GetNextId(base::BindRepeating(
      &DownloadIdCallback, download_manager, path, url_chain, mime_type, offset,
      length, last_modified, etag, base::Time::FromDoubleT(start_time)));
}

void Session::SetPreloads(
    const std::vector<base::FilePath::StringType>& preloads) {
  auto* prefs = SessionPreferences::FromBrowserContext(browser_context());
  DCHECK(prefs);
  prefs->set_preloads(preloads);
}

std::vector<base::FilePath::StringType> Session::GetPreloads() const {
  auto* prefs = SessionPreferences::FromBrowserContext(browser_context());
  DCHECK(prefs);
  return prefs->preloads();
}

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
v8::Local<v8::Promise> Session::LoadExtension(
    const base::FilePath& extension_path) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  gin_helper::Promise<const extensions::Extension*> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  if (browser_context()->IsOffTheRecord()) {
    promise.RejectWithErrorMessage(
        "Extensions cannot be loaded in a temporary session");
    return handle;
  }

  auto* extension_system = static_cast<extensions::ElectronExtensionSystem*>(
      extensions::ExtensionSystem::Get(browser_context()));
  extension_system->LoadExtension(
      extension_path,
      base::BindOnce(
          [](gin_helper::Promise<const extensions::Extension*> promise,
             const extensions::Extension* extension,
             const std::string& error_msg) {
            if (extension) {
              if (!error_msg.empty()) {
                node::Environment* env =
                    node::Environment::GetCurrent(v8::Isolate::GetCurrent());
                EmitWarning(env, error_msg, "ExtensionLoadWarning");
              }
              promise.Resolve(extension);
            } else {
              promise.RejectWithErrorMessage(error_msg);
            }
          },
          std::move(promise)));

  return handle;
}

void Session::RemoveExtension(const std::string& extension_id) {
  auto* extension_system = static_cast<extensions::ElectronExtensionSystem*>(
      extensions::ExtensionSystem::Get(browser_context()));
  extension_system->RemoveExtension(extension_id);
}

v8::Local<v8::Value> Session::GetExtension(const std::string& extension_id) {
  auto* registry = extensions::ExtensionRegistry::Get(browser_context());
  const extensions::Extension* extension =
      registry->GetInstalledExtension(extension_id);
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (extension) {
    return gin::ConvertToV8(isolate, extension);
  } else {
    return v8::Null(isolate);
  }
}

v8::Local<v8::Value> Session::GetAllExtensions() {
  auto* registry = extensions::ExtensionRegistry::Get(browser_context());
  auto installed_extensions = registry->GenerateInstalledExtensionsSet();
  std::vector<const extensions::Extension*> extensions_vector;
  for (const auto& extension : *installed_extensions) {
    if (extension->location() != extensions::Manifest::COMPONENT)
      extensions_vector.emplace_back(extension.get());
  }
  return gin::ConvertToV8(v8::Isolate::GetCurrent(), extensions_vector);
}
#endif

v8::Local<v8::Value> Session::Cookies(v8::Isolate* isolate) {
  if (cookies_.IsEmpty()) {
    auto handle = Cookies::Create(isolate, browser_context());
    cookies_.Reset(isolate, handle.ToV8());
  }
  return cookies_.Get(isolate);
}

v8::Local<v8::Value> Session::Protocol(v8::Isolate* isolate) {
  return protocol_.Get(isolate);
}

v8::Local<v8::Value> Session::ServiceWorkerContext(v8::Isolate* isolate) {
  if (service_worker_context_.IsEmpty()) {
    v8::Local<v8::Value> handle;
    handle = ServiceWorkerContext::Create(isolate, browser_context()).ToV8();
    service_worker_context_.Reset(isolate, handle);
  }
  return service_worker_context_.Get(isolate);
}

v8::Local<v8::Value> Session::WebRequest(v8::Isolate* isolate) {
  if (web_request_.IsEmpty()) {
    auto handle = WebRequest::Create(isolate, browser_context());
    web_request_.Reset(isolate, handle.ToV8());
  }
  return web_request_.Get(isolate);
}

v8::Local<v8::Value> Session::NetLog(v8::Isolate* isolate) {
  if (net_log_.IsEmpty()) {
    auto handle = NetLog::Create(isolate, browser_context());
    net_log_.Reset(isolate, handle.ToV8());
  }
  return net_log_.Get(isolate);
}

static void StartPreconnectOnUI(
    scoped_refptr<ElectronBrowserContext> browser_context,
    const GURL& url,
    int num_sockets_to_preconnect) {
  std::vector<predictors::PreconnectRequest> requests = {
      {url::Origin::Create(url), num_sockets_to_preconnect,
       net::NetworkIsolationKey()}};
  browser_context->GetPreconnectManager()->Start(url, requests);
}

void Session::Preconnect(const gin_helper::Dictionary& options,
                         gin::Arguments* args) {
  GURL url;
  if (!options.Get("url", &url) || !url.is_valid()) {
    args->ThrowTypeError(
        "Must pass non-empty valid url to session.preconnect.");
    return;
  }
  int num_sockets_to_preconnect = 1;
  if (options.Get("numSockets", &num_sockets_to_preconnect)) {
    const int kMinSocketsToPreconnect = 1;
    const int kMaxSocketsToPreconnect = 6;
    if (num_sockets_to_preconnect < kMinSocketsToPreconnect ||
        num_sockets_to_preconnect > kMaxSocketsToPreconnect) {
      args->ThrowTypeError(
          base::StringPrintf("numSocketsToPreconnect is outside range [%d,%d]",
                             kMinSocketsToPreconnect, kMaxSocketsToPreconnect));
      return;
    }
  }

  DCHECK_GT(num_sockets_to_preconnect, 0);
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&StartPreconnectOnUI, base::RetainedRef(browser_context_),
                     url, num_sockets_to_preconnect));
}

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
base::Value Session::GetSpellCheckerLanguages() {
  return browser_context_->prefs()
      ->Get(spellcheck::prefs::kSpellCheckDictionaries)
      ->Clone();
}

void Session::SetSpellCheckerLanguages(
    gin_helper::ErrorThrower thrower,
    const std::vector<std::string>& languages) {
  base::ListValue language_codes;
  for (const std::string& lang : languages) {
    std::string code = spellcheck::GetCorrespondingSpellCheckLanguage(lang);
    if (code.empty()) {
      thrower.ThrowError("Invalid language code provided: \"" + lang +
                         "\" is not a valid language code");
      return;
    }
    language_codes.AppendString(code);
  }
  browser_context_->prefs()->Set(spellcheck::prefs::kSpellCheckDictionaries,
                                 language_codes);
}

void SetSpellCheckerDictionaryDownloadURL(gin_helper::ErrorThrower thrower,
                                          const GURL& url) {
  if (!url.is_valid()) {
    thrower.ThrowError(
        "The URL you provided to setSpellCheckerDictionaryDownloadURL is not a "
        "valid URL");
    return;
  }
  SpellcheckHunspellDictionary::SetBaseDownloadURL(url);
}

v8::Local<v8::Promise> Session::ListWordsInSpellCheckerDictionary() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  gin_helper::Promise<std::set<std::string>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  SpellcheckService* spellcheck =
      SpellcheckServiceFactory::GetForContext(browser_context_.get());

  if (!spellcheck)
    promise.RejectWithErrorMessage(
        "Spellcheck in unexpected state: failed to load custom dictionary.");

  if (spellcheck->GetCustomDictionary()->IsLoaded()) {
    promise.Resolve(spellcheck->GetCustomDictionary()->GetWords());
  } else {
    new DictionaryObserver(std::move(promise), spellcheck->GetWeakPtr());
    // Dictionary loads by default asynchronously,
    // call the load function anyways just to be sure.
    spellcheck->GetCustomDictionary()->Load();
  }

  return handle;
}

bool Session::AddWordToSpellCheckerDictionary(const std::string& word) {
  // don't let in-memory sessions add spellchecker words
  // because files will persist unintentionally
  bool is_in_memory = browser_context_->IsOffTheRecord();
  if (is_in_memory)
    return false;

  SpellcheckService* service =
      SpellcheckServiceFactory::GetForContext(browser_context_.get());
  if (!service)
    return false;

#if BUILDFLAG(USE_BROWSER_SPELLCHECKER)
  if (spellcheck::UseBrowserSpellChecker()) {
    spellcheck_platform::AddWord(service->platform_spell_checker(),
                                 base::UTF8ToUTF16(word));
  }
#endif
  return service->GetCustomDictionary()->AddWord(word);
}

bool Session::RemoveWordFromSpellCheckerDictionary(const std::string& word) {
  // don't let in-memory sessions remove spellchecker words
  // because files will persist unintentionally
  bool is_in_memory = browser_context_->IsOffTheRecord();
  if (is_in_memory)
    return false;

  SpellcheckService* service =
      SpellcheckServiceFactory::GetForContext(browser_context_.get());
  if (!service)
    return false;

#if BUILDFLAG(USE_BROWSER_SPELLCHECKER)
  if (spellcheck::UseBrowserSpellChecker()) {
    spellcheck_platform::RemoveWord(service->platform_spell_checker(),
                                    base::UTF8ToUTF16(word));
  }
#endif
  return service->GetCustomDictionary()->RemoveWord(word);
}
#endif  // BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)

// static
Session* Session::FromBrowserContext(content::BrowserContext* context) {
  UserDataLink* data =
      static_cast<UserDataLink*>(context->GetUserData(kElectronApiSessionKey));
  return data ? data->session : nullptr;
}

// static
gin::Handle<Session> Session::CreateFrom(
    v8::Isolate* isolate,
    ElectronBrowserContext* browser_context) {
  Session* existing = FromBrowserContext(browser_context);
  if (existing)
    return gin::CreateHandle(isolate, existing);

  auto handle =
      gin::CreateHandle(isolate, new Session(isolate, browser_context));

  // The Sessions should never be garbage collected, since the common pattern is
  // to use partition strings, instead of using the Session object directly.
  handle->Pin(isolate);
  ElectronBrowserMainParts::Get()->RegisterDestructionCallback(
      base::BindOnce([](Session* session) { delete session; }, handle.get()));

  App::Get()->EmitCustomEvent("session-created",
                              handle.ToV8().As<v8::Object>());

  return handle;
}

// static
gin::Handle<Session> Session::FromPartition(v8::Isolate* isolate,
                                            const std::string& partition,
                                            base::DictionaryValue options) {
  scoped_refptr<ElectronBrowserContext> browser_context;
  if (partition.empty()) {
    browser_context =
        ElectronBrowserContext::From("", false, std::move(options));
  } else if (base::StartsWith(partition, kPersistPrefix,
                              base::CompareCase::SENSITIVE)) {
    std::string name = partition.substr(8);
    browser_context =
        ElectronBrowserContext::From(name, false, std::move(options));
  } else {
    browser_context =
        ElectronBrowserContext::From(partition, true, std::move(options));
  }
  return CreateFrom(isolate, browser_context.get());
}

gin::ObjectTemplateBuilder Session::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<Session>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("resolveProxy", &Session::ResolveProxy)
      .SetMethod("getCacheSize", &Session::GetCacheSize)
      .SetMethod("clearCache", &Session::ClearCache)
      .SetMethod("clearStorageData", &Session::ClearStorageData)
      .SetMethod("flushStorageData", &Session::FlushStorageData)
      .SetMethod("setProxy", &Session::SetProxy)
      .SetMethod("setDownloadPath", &Session::SetDownloadPath)
      .SetMethod("enableNetworkEmulation", &Session::EnableNetworkEmulation)
      .SetMethod("disableNetworkEmulation", &Session::DisableNetworkEmulation)
      .SetMethod("setCertificateVerifyProc", &Session::SetCertVerifyProc)
      .SetMethod("setPermissionRequestHandler",
                 &Session::SetPermissionRequestHandler)
      .SetMethod("setPermissionCheckHandler",
                 &Session::SetPermissionCheckHandler)
      .SetMethod("clearHostResolverCache", &Session::ClearHostResolverCache)
      .SetMethod("clearAuthCache", &Session::ClearAuthCache)
      .SetMethod("allowNTLMCredentialsForDomains",
                 &Session::AllowNTLMCredentialsForDomains)
      .SetMethod("isPersistent", &Session::IsPersistent)
      .SetMethod("setUserAgent", &Session::SetUserAgent)
      .SetMethod("getUserAgent", &Session::GetUserAgent)
      .SetMethod("getBlobData", &Session::GetBlobData)
      .SetMethod("downloadURL", &Session::DownloadURL)
      .SetMethod("createInterruptedDownload",
                 &Session::CreateInterruptedDownload)
      .SetMethod("setPreloads", &Session::SetPreloads)
      .SetMethod("getPreloads", &Session::GetPreloads)
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
      .SetMethod("loadExtension", &Session::LoadExtension)
      .SetMethod("removeExtension", &Session::RemoveExtension)
      .SetMethod("getExtension", &Session::GetExtension)
      .SetMethod("getAllExtensions", &Session::GetAllExtensions)
#endif
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
      .SetMethod("getSpellCheckerLanguages", &Session::GetSpellCheckerLanguages)
      .SetMethod("setSpellCheckerLanguages", &Session::SetSpellCheckerLanguages)
      .SetProperty("availableSpellCheckerLanguages",
                   &spellcheck::SpellCheckLanguages)
      .SetMethod("setSpellCheckerDictionaryDownloadURL",
                 &SetSpellCheckerDictionaryDownloadURL)
      .SetMethod("listWordsInSpellCheckerDictionary",
                 &Session::ListWordsInSpellCheckerDictionary)
      .SetMethod("addWordToSpellCheckerDictionary",
                 &Session::AddWordToSpellCheckerDictionary)
      .SetMethod("removeWordFromSpellCheckerDictionary",
                 &Session::RemoveWordFromSpellCheckerDictionary)
#endif
      .SetMethod("preconnect", &Session::Preconnect)
      .SetProperty("cookies", &Session::Cookies)
      .SetProperty("netLog", &Session::NetLog)
      .SetProperty("protocol", &Session::Protocol)
      .SetProperty("serviceWorkers", &Session::ServiceWorkerContext)
      .SetProperty("webRequest", &Session::WebRequest);
}

const char* Session::GetTypeName() {
  return "Session";
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::Cookies;
using electron::api::Protocol;
using electron::api::ServiceWorkerContext;
using electron::api::Session;

v8::Local<v8::Value> FromPartition(const std::string& partition,
                                   gin::Arguments* args) {
  if (!electron::Browser::Get()->is_ready()) {
    args->ThrowTypeError("Session can only be received when app is ready");
    return v8::Null(args->isolate());
  }
  base::DictionaryValue options;
  args->GetNext(&options);
  return Session::FromPartition(args->isolate(), partition, std::move(options))
      .ToV8();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("fromPartition", &FromPartition);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_session, Initialize)
