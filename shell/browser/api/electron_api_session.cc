// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_session.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/scoped_observation.h"
#include "base/strings/string_util.h"
#include "base/types/pass_key.h"
#include "base/uuid.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/predictors/predictors_traffic_annotations.h"  // nogncheck
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/download/public/common/download_danger_type.h"
#include "components/download/public/common/download_url_parameters.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/value_map_pref_store.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "components/proxy_config/proxy_prefs.h"
#include "content/browser/code_cache/generated_code_cache_context.h"  // nogncheck
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browsing_data_filter_builder.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/download_manager_delegate.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/preconnect_manager.h"
#include "content/public/browser/preconnect_request.h"
#include "content/public/browser/storage_partition.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/completion_repeating_callback.h"
#include "net/base/load_flags.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_cache.h"
#include "net/http/http_util.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/request_destination.h"
#include "services/network/public/mojom/clear_data_filter.mojom.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/browser/api/electron_api_cookies.h"
#include "shell/browser/api/electron_api_data_pipe_holder.h"
#include "shell/browser/api/electron_api_download_item.h"
#include "shell/browser/api/electron_api_net_log.h"
#include "shell/browser/api/electron_api_protocol.h"
#include "shell/browser/api/electron_api_service_worker_context.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/browser/api/electron_api_web_request.h"
#include "shell/browser/browser.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/electron_permission_manager.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/media/media_device_id_salt.h"
#include "shell/browser/net/cert_verifier_client.h"
#include "shell/browser/net/resolve_host_function.h"
#include "shell/browser/session_preferences.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/media_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_converters/time_converter.h"
#include "shell/common/gin_converters/usb_protected_classes_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/options_switches.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"
#include "v8/include/v8-traced-handle.h"

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "shell/browser/api/electron_api_extensions.h"
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
using content::BrowsingDataFilterBuilder;
using content::BrowsingDataRemover;
using content::StoragePartition;

namespace {

struct ClearStorageDataOptions {
  blink::StorageKey storage_key;
  uint32_t storage_types = StoragePartition::REMOVE_DATA_MASK_ALL;
  uint32_t quota_types = StoragePartition::QUOTA_MANAGED_STORAGE_MASK_ALL;
};

uint32_t GetStorageMask(const std::vector<std::string>& storage_types) {
  static constexpr auto Lookup =
      base::MakeFixedFlatMap<std::string_view, uint32_t>(
          {{"cookies", StoragePartition::REMOVE_DATA_MASK_COOKIES},
           {"filesystem", StoragePartition::REMOVE_DATA_MASK_FILE_SYSTEMS},
           {"indexdb", StoragePartition::REMOVE_DATA_MASK_INDEXEDDB},
           {"localstorage", StoragePartition::REMOVE_DATA_MASK_LOCAL_STORAGE},
           {"shadercache", StoragePartition::REMOVE_DATA_MASK_SHADER_CACHE},
           {"serviceworkers",
            StoragePartition::REMOVE_DATA_MASK_SERVICE_WORKERS},
           {"cachestorage", StoragePartition::REMOVE_DATA_MASK_CACHE_STORAGE}});

  uint32_t storage_mask = 0;
  for (const auto& it : storage_types) {
    if (const uint32_t* val = base::FindOrNull(Lookup, base::ToLowerASCII(it)))
      storage_mask |= *val;
  }
  return storage_mask;
}

uint32_t GetQuotaMask(const std::vector<std::string>& quota_types) {
  uint32_t quota_mask = 0;
  for (const auto& it : quota_types) {
    auto type = base::ToLowerASCII(it);
    if (type == "temporary")
      quota_mask |= StoragePartition::QUOTA_MANAGED_STORAGE_MASK_TEMPORARY;
  }
  return quota_mask;
}

constexpr BrowsingDataRemover::DataType kClearDataTypeAll =
    ~0ULL & ~BrowsingDataRemover::DATA_TYPE_AVOID_CLOSING_CONNECTIONS;
constexpr BrowsingDataRemover::OriginType kClearOriginTypeAll =
    BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
    BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB;

constexpr auto kDataTypeLookup =
    base::MakeFixedFlatMap<std::string_view, BrowsingDataRemover::DataType>({
        {"backgroundFetch", BrowsingDataRemover::DATA_TYPE_BACKGROUND_FETCH},
        {"cache", BrowsingDataRemover::DATA_TYPE_CACHE |
                      BrowsingDataRemover::DATA_TYPE_CACHE_STORAGE},
        {"cookies", BrowsingDataRemover::DATA_TYPE_COOKIES},
        {"downloads", BrowsingDataRemover::DATA_TYPE_DOWNLOADS},
        {"fileSystems", BrowsingDataRemover::DATA_TYPE_FILE_SYSTEMS},
        {"indexedDB", BrowsingDataRemover::DATA_TYPE_INDEXED_DB},
        {"localStorage", BrowsingDataRemover::DATA_TYPE_LOCAL_STORAGE},
        {"serviceWorkers", BrowsingDataRemover::DATA_TYPE_SERVICE_WORKERS},
    });

BrowsingDataRemover::DataType GetDataTypeMask(
    const std::vector<std::string>& data_types) {
  BrowsingDataRemover::DataType mask = 0u;
  for (const auto& type : data_types) {
    if (const auto* val = base::FindOrNull(kDataTypeLookup, type))
      mask |= *val;
  }
  return mask;
}

std::vector<std::string> GetDataTypesFromMask(
    BrowsingDataRemover::DataType mask) {
  std::vector<std::string> results;
  for (const auto [type, flag] : kDataTypeLookup) {
    if (mask & flag) {
      results.emplace_back(type);
    }
  }
  return results;
}

// Represents a task to clear browsing data for the `clearData` API method.
//
// This type manages its own lifetime,
// 1) deleting itself once all the operations created by this task are
// completed. 2) through gin_helper::CleanedUpAtExit, ensuring it's destroyed
// before the node environment shuts down.
class ClearDataTask : public gin_helper::CleanedUpAtExit {
 public:
  // Starts running a task. This function will return before the task is
  // finished, but will resolve or reject the |promise| when it finishes.
  static void Run(
      BrowsingDataRemover* remover,
      gin_helper::Promise<void> promise,
      BrowsingDataRemover::DataType data_type_mask,
      std::vector<url::Origin> origins,
      BrowsingDataFilterBuilder::Mode filter_mode,
      BrowsingDataFilterBuilder::OriginMatchingMode origin_matching_mode) {
    auto* task = new ClearDataTask(std::move(promise));

    // This method counts as an operation. This is important so we can call
    // `OnOperationFinished` at the end of this method as a fallback if all the
    // other operations finished while this method was still executing
    task->operations_running_ = 1;

    // Cookies are scoped more broadly than other types of data, so if we are
    // filtering then we need to do it at the registrable domain level
    if (!origins.empty() &&
        data_type_mask & BrowsingDataRemover::DATA_TYPE_COOKIES) {
      data_type_mask &= ~BrowsingDataRemover::DATA_TYPE_COOKIES;

      auto cookies_filter_builder =
          BrowsingDataFilterBuilder::Create(filter_mode);

      for (const url::Origin& origin : origins) {
        std::string domain = GetDomainAndRegistry(
            origin,
            net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
        if (domain.empty()) {
          domain = origin.host();
        }
        cookies_filter_builder->AddRegisterableDomain(domain);
      }

      StartOperation(task, remover, BrowsingDataRemover::DATA_TYPE_COOKIES,
                     std::move(cookies_filter_builder));
    }

    // If cookies aren't the only data type and weren't handled above, then we
    // can start an operation that is scoped to origins
    if (data_type_mask) {
      auto filter_builder =
          BrowsingDataFilterBuilder::Create(filter_mode, origin_matching_mode);

      for (auto const& origin : origins) {
        filter_builder->AddOrigin(origin);
      }

      StartOperation(task, remover, data_type_mask, std::move(filter_builder));
    }

    // This static method counts as an operation.
    task->OnOperationFinished(nullptr, std::nullopt);
  }

 private:
  // An individual |content::BrowsingDataRemover::Remove...| operation as part
  // of a full |ClearDataTask|. This class is owned by ClearDataTask and cleaned
  // up either when the operation completes or when ClearDataTask is destroyed.
  class ClearDataOperation : private BrowsingDataRemover::Observer {
   public:
    ClearDataOperation(ClearDataTask* task, BrowsingDataRemover* remover)
        : task_(task) {
      observation_.Observe(remover);
    }

    void Start(BrowsingDataRemover* remover,
               BrowsingDataRemover::DataType data_type_mask,
               std::unique_ptr<BrowsingDataFilterBuilder> filter_builder) {
      remover->RemoveWithFilterAndReply(base::Time::Min(), base::Time::Max(),
                                        data_type_mask, kClearOriginTypeAll,
                                        std::move(filter_builder), this);
    }

    // BrowsingDataRemover::Observer:
    void OnBrowsingDataRemoverDone(
        BrowsingDataRemover::DataType failed_data_types) override {
      task_->OnOperationFinished(this, failed_data_types);
    }

   private:
    raw_ptr<ClearDataTask> task_;
    base::ScopedObservation<BrowsingDataRemover, BrowsingDataRemover::Observer>
        observation_{this};
  };

  explicit ClearDataTask(gin_helper::Promise<void> promise)
      : promise_(std::move(promise)) {}

  static void StartOperation(
      ClearDataTask* task,
      BrowsingDataRemover* remover,
      BrowsingDataRemover::DataType data_type_mask,
      std::unique_ptr<BrowsingDataFilterBuilder> filter_builder) {
    // Track this operation
    task->operations_running_ += 1;

    auto& operation = task->operations_.emplace_back(
        std::make_unique<ClearDataOperation>(task, remover));
    operation->Start(remover, data_type_mask, std::move(filter_builder));
  }

  void OnOperationFinished(
      ClearDataOperation* operation,
      std::optional<BrowsingDataRemover::DataType> failed_data_types) {
    DCHECK_GT(operations_running_, 0);
    operations_running_ -= 1;

    if (failed_data_types.has_value()) {
      failed_data_types_ |= failed_data_types.value();
    }

    if (operation) {
      operations_.erase(
          std::remove_if(
              operations_.begin(), operations_.end(),
              [operation](const std::unique_ptr<ClearDataOperation>& op) {
                return op.get() == operation;
              }),
          operations_.end());
    }

    // If this is the last operation, then the task is finished
    if (operations_running_ == 0) {
      OnTaskFinished();
    }
  }

  void OnTaskFinished() {
    if (failed_data_types_ == 0ULL) {
      promise_.Resolve();
    } else {
      v8::Isolate* isolate = promise_.isolate();

      v8::Local<v8::Value> failed_data_types_array =
          gin::ConvertToV8(isolate, GetDataTypesFromMask(failed_data_types_));

      // Create a rich error object with extra detail about what data types
      // failed
      auto error = v8::Exception::Error(
          gin::StringToV8(isolate, "Failed to clear data"));
      error.As<v8::Object>()
          ->Set(promise_.GetContext(),
                gin::StringToV8(isolate, "failedDataTypes"),
                failed_data_types_array)
          .Check();

      promise_.Reject(error);
    }

    delete this;
  }

  int operations_running_ = 0;
  BrowsingDataRemover::DataType failed_data_types_ = 0ULL;
  gin_helper::Promise<void> promise_;
  std::vector<std::unique_ptr<ClearDataOperation>> operations_;
};

base::DictValue createProxyConfig(ProxyPrefs::ProxyMode proxy_mode,
                                  std::string const& pac_url,
                                  std::string const& proxy_server,
                                  std::string const& bypass_list) {
  if (proxy_mode == ProxyPrefs::MODE_DIRECT) {
    return ProxyConfigDictionary::CreateDirect();
  }

  if (proxy_mode == ProxyPrefs::MODE_SYSTEM) {
    return ProxyConfigDictionary::CreateSystem();
  }

  if (proxy_mode == ProxyPrefs::MODE_AUTO_DETECT) {
    return ProxyConfigDictionary::CreateAutoDetect();
  }

  if (proxy_mode == ProxyPrefs::MODE_PAC_SCRIPT) {
    const bool pac_mandatory = true;
    return ProxyConfigDictionary::CreatePacScript(pac_url, pac_mandatory);
  }

  return ProxyConfigDictionary::CreateFixedServers(proxy_server, bypass_list);
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
    if (GURL storage_origin; options.Get("origin", &storage_origin))
      out->storage_key = blink::StorageKey::CreateFirstParty(
          url::Origin::Create(storage_origin));
    std::vector<std::string> types;
    if (options.Get("storages", &types))
      out->storage_types = GetStorageMask(types);
    if (options.Get("quotas", &types))
      out->quota_types = GetQuotaMask(types);
    return true;
  }
};

bool SSLProtocolVersionFromString(const std::string& version_str,
                                  network::mojom::SSLVersion* version) {
  if (version_str == switches::kSSLVersionTLSv12) {
    *version = network::mojom::SSLVersion::kTLS12;
    return true;
  }
  if (version_str == switches::kSSLVersionTLSv13) {
    *version = network::mojom::SSLVersion::kTLS13;
    return true;
  }
  return false;
}

template <>
struct Converter<uint16_t> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     uint16_t* out) {
    auto maybe = val->IntegerValue(isolate->GetCurrentContext());
    if (maybe.IsNothing())
      return false;
    *out = maybe.FromJust();
    return true;
  }
};

template <>
struct Converter<network::mojom::SSLConfigPtr> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     network::mojom::SSLConfigPtr* out) {
    gin_helper::Dictionary options;
    if (!ConvertFromV8(isolate, val, &options))
      return false;
    *out = network::mojom::SSLConfig::New();
    std::string version_min_str;
    if (options.Get("minVersion", &version_min_str)) {
      if (!SSLProtocolVersionFromString(version_min_str, &(*out)->version_min))
        return false;
    }
    std::string version_max_str;
    if (options.Get("maxVersion", &version_max_str)) {
      if (!SSLProtocolVersionFromString(version_max_str,
                                        &(*out)->version_max) ||
          (*out)->version_max < network::mojom::SSLVersion::kTLS12)
        return false;
    }

    if (options.Has("disabledCipherSuites") &&
        !options.Get("disabledCipherSuites", &(*out)->disabled_cipher_suites)) {
      return false;
    }
    std::ranges::sort((*out)->disabled_cipher_suites);

    // TODO(nornagon): also support other SSLConfig properties?
    return true;
  }
};

}  // namespace gin

namespace electron::api {

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
      base::Uuid::GenerateRandomV4().AsLowercaseString(), id, path, path,
      url_chain, GURL(),
      content::StoragePartitionConfig::CreateDefault(
          download_manager->GetBrowserContext()),
      GURL(), GURL(), std::nullopt, mime_type, mime_type, start_time,
      base::Time(), etag, last_modified, offset, length, std::string(),
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
  explicit UserDataLink(
      cppgc::WeakPersistent<gin::WeakCell<Session>> session_in)
      : session{session_in} {}

  cppgc::WeakPersistent<gin::WeakCell<Session>> session;
};

const void* kElectronApiSessionKey = &kElectronApiSessionKey;

}  // namespace

gin::WrapperInfo Session::kWrapperInfo = {{gin::kEmbedderNativeGin},
                                          gin::kElectronSession};

Session::Session(v8::Isolate* isolate, ElectronBrowserContext* browser_context)
    : isolate_(isolate),
      network_emulation_token_(base::UnguessableToken::Create()),
      browser_context_{
          raw_ref<ElectronBrowserContext>::from_ptr(browser_context)} {
  gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
  data->AddDisposeObserver(this);
  // Observe DownloadManager to get download notifications.
  browser_context->GetDownloadManager()->AddObserver(this);

  SessionPreferences::CreateForBrowserContext(browser_context);

  protocol_.Reset(
      isolate,
      Protocol::Create(isolate, browser_context->protocol_registry()).ToV8());

  browser_context->SetUserData(
      kElectronApiSessionKey,
      std::make_unique<UserDataLink>(
          cppgc::WeakPersistent<gin::WeakCell<Session>>(
              weak_factory_.GetWeakCell(
                  isolate->GetCppHeap()->GetAllocationHandle()))));

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  if (auto* service =
          SpellcheckServiceFactory::GetForContext(browser_context)) {
    service->SetHunspellObserver(this);
    service->InitializeDictionaries(base::DoNothing());
  }
#endif
}

Session::~Session() {
  Dispose();
}

void Session::Dispose() {
  if (keep_alive_) {
    browser_context()->GetDownloadManager()->RemoveObserver(this);

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
    if (auto* service =
            SpellcheckServiceFactory::GetForContext(browser_context())) {
      service->SetHunspellObserver(nullptr);
    }
#endif
  }
}

void Session::OnDownloadCreated(content::DownloadManager* manager,
                                download::DownloadItem* item) {
  if (item->IsSavePackageDownload())
    return;

  v8::HandleScope handle_scope(isolate_);
  auto handle = DownloadItem::FromOrCreate(isolate_, item);
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

v8::Local<v8::Promise> Session::ResolveHost(
    std::string host,
    std::optional<network::mojom::ResolveHostParametersPtr> params) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate_);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto fn = base::MakeRefCounted<ResolveHostFunction>(
      browser_context(), std::move(host),
      params ? std::move(params.value()) : nullptr,
      base::BindOnce(
          [](gin_helper::Promise<gin_helper::Dictionary> promise,
             int64_t net_error, const std::optional<net::AddressList>& addrs) {
            if (net_error < 0) {
              promise.RejectWithErrorMessage(net::ErrorToString(net_error));
            } else {
              DCHECK(addrs.has_value() && !addrs->empty());

              v8::HandleScope handle_scope(promise.isolate());
              auto dict =
                  gin_helper::Dictionary::CreateEmpty(promise.isolate());
              dict.Set("endpoints", addrs->endpoints());
              promise.Resolve(dict);
            }
          },
          std::move(promise)));

  fn->Run();

  return handle;
}

v8::Local<v8::Promise> Session::GetCacheSize() {
  gin_helper::Promise<int64_t> promise(isolate_);
  auto handle = promise.GetHandle();

  browser_context_->GetDefaultStoragePartition()
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
  gin_helper::Promise<void> promise(isolate_);
  auto handle = promise.GetHandle();

  browser_context_->GetDefaultStoragePartition()
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

  if (options.storage_types & StoragePartition::REMOVE_DATA_MASK_COOKIES) {
    // Reset media device id salt when cookies are cleared.
    // https://w3c.github.io/mediacapture-main/#dom-mediadeviceinfo-deviceid
    MediaDeviceIDSalt::Reset(browser_context()->prefs());
  }

  browser_context()->GetDefaultStoragePartition()->ClearData(
      options.storage_types, options.quota_types, options.storage_key,
      base::Time(), base::Time::Max(),
      base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                     std::move(promise)));
  return handle;
}

void Session::FlushStorageData() {
  browser_context()->GetDefaultStoragePartition()->Flush();
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

  std::string mode, proxy_rules, bypass_list, pac_url;

  options.Get("pacScript", &pac_url);
  options.Get("proxyRules", &proxy_rules);
  options.Get("proxyBypassRules", &bypass_list);

  ProxyPrefs::ProxyMode proxy_mode = ProxyPrefs::MODE_FIXED_SERVERS;
  if (!options.Get("mode", &mode)) {
    // pacScript takes precedence over proxyRules.
    if (!pac_url.empty()) {
      proxy_mode = ProxyPrefs::MODE_PAC_SCRIPT;
    } else {
      proxy_mode = ProxyPrefs::MODE_FIXED_SERVERS;
    }
  } else {
    if (!ProxyPrefs::StringToProxyMode(mode, &proxy_mode)) {
      promise.RejectWithErrorMessage(
          "Invalid mode, must be one of direct, auto_detect, pac_script, "
          "fixed_servers or system");
      return handle;
    }
  }

  browser_context_->in_memory_pref_store()->SetValue(
      proxy_config::prefs::kProxy,
      base::Value{
          createProxyConfig(proxy_mode, pac_url, proxy_rules, bypass_list)},
      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);

  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                                std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> Session::ForceReloadProxyConfig() {
  gin_helper::Promise<void> promise(isolate_);
  auto handle = promise.GetHandle();

  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->ForceReloadProxyConfig(base::BindOnce(
          gin_helper::Promise<void>::ResolvePromise, std::move(promise)));

  return handle;
}

void Session::SetDownloadPath(const base::FilePath& path) {
  browser_context_->prefs()->SetFilePath(prefs::kDownloadDefaultDirectory,
                                         path);
}

void Session::EnableNetworkEmulation(const gin_helper::Dictionary& options) {
  std::vector<network::mojom::MatchedNetworkConditionsPtr> matched_conditions;
  network::mojom::MatchedNetworkConditionsPtr network_conditions =
      network::mojom::MatchedNetworkConditions::New();
  network_conditions->conditions = network::mojom::NetworkConditions::New();
  options.Get("offline", &network_conditions->conditions->offline);
  options.Get("downloadThroughput",
              &network_conditions->conditions->download_throughput);
  options.Get("uploadThroughput",
              &network_conditions->conditions->upload_throughput);
  double latency = 0.0;
  if (options.Get("latency", &latency) && latency) {
    network_conditions->conditions->latency = base::Milliseconds(latency);
  }
  matched_conditions.emplace_back(std::move(network_conditions));

  auto* network_context =
      browser_context_->GetDefaultStoragePartition()->GetNetworkContext();
  network_context->SetNetworkConditions(network_emulation_token_,
                                        std::move(matched_conditions));
}

void Session::DisableNetworkEmulation() {
  auto* network_context =
      browser_context_->GetDefaultStoragePartition()->GetNetworkContext();
  std::vector<network::mojom::MatchedNetworkConditionsPtr> network_conditions;
  network_context->SetNetworkConditions(network_emulation_token_,
                                        std::move(network_conditions));
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
  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->SetCertVerifierClient(std::move(cert_verifier_client_remote));
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
         blink::PermissionType permission_type,
         ElectronPermissionManager::StatusCallback callback,
         const base::Value& details) {
#if (BUILDFLAG(IS_MAC))
        if (permission_type == blink::PermissionType::GEOLOCATION) {
          if (ElectronPermissionManager::
                  IsGeolocationDisabledViaCommandLine()) {
            auto original_callback = std::move(callback);
            callback = base::BindOnce(
                [](ElectronPermissionManager::StatusCallback callback,
                   content::PermissionResult /*ignored_result*/) {
                  // Always deny regardless of what
                  // content::PermissionResult is passed here
                  std::move(callback).Run(content::PermissionResult(
                      blink::mojom::PermissionStatus::DENIED,
                      content::PermissionStatusSource::UNSPECIFIED));
                },
                std::move(original_callback));
          }
        }
#endif
        handler->Run(web_contents, permission_type, std::move(callback),
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

void Session::SetDisplayMediaRequestHandler(v8::Isolate* isolate,
                                            v8::Local<v8::Value> val) {
  if (val->IsNull()) {
    browser_context_->SetDisplayMediaRequestHandler(
        DisplayMediaRequestHandler());
    return;
  }
  DisplayMediaRequestHandler handler;
  if (!gin::ConvertFromV8(isolate, val, &handler)) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "Display media request handler must be null or a function");
    return;
  }
  browser_context_->SetDisplayMediaRequestHandler(handler);
}

void Session::SetDevicePermissionHandler(v8::Local<v8::Value> val,
                                         gin::Arguments* args) {
  ElectronPermissionManager::DeviceCheckHandler handler;
  if (!(val->IsNull() || gin::ConvertFromV8(args->isolate(), val, &handler))) {
    args->ThrowTypeError("Must pass null or function");
    return;
  }
  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context()->GetPermissionControllerDelegate());
  permission_manager->SetDevicePermissionHandler(handler);
}

void Session::SetUSBProtectedClassesHandler(v8::Local<v8::Value> val,
                                            gin::Arguments* args) {
  ElectronPermissionManager::ProtectedUSBHandler handler;
  if (!(val->IsNull() || gin::ConvertFromV8(args->isolate(), val, &handler))) {
    args->ThrowTypeError("Must pass null or function");
    return;
  }
  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context()->GetPermissionControllerDelegate());
  permission_manager->SetProtectedUSBHandler(handler);
}

void Session::SetBluetoothPairingHandler(v8::Local<v8::Value> val,
                                         gin::Arguments* args) {
  ElectronPermissionManager::BluetoothPairingHandler handler;
  if (!(val->IsNull() || gin::ConvertFromV8(args->isolate(), val, &handler))) {
    args->ThrowTypeError("Must pass null or function");
    return;
  }
  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context()->GetPermissionControllerDelegate());
  permission_manager->SetBluetoothPairingHandler(handler);
}

v8::Local<v8::Promise> Session::ClearHostResolverCache(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->ClearHostCache(nullptr,
                       base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                                      std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> Session::ClearAuthCache() {
  gin_helper::Promise<void> promise(isolate_);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->ClearHttpAuthCache(
          base::Time(), base::Time::Max(),
          nullptr /*mojom::ClearDataFilterPtr*/,
          base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                         std::move(promise)));

  return handle;
}

void Session::AllowNTLMCredentialsForDomains(const std::string& domains) {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  network::mojom::HttpAuthDynamicParamsPtr auth_dynamic_params =
      network::mojom::HttpAuthDynamicParams::New();
  auth_dynamic_params->server_allowlist = domains;
  auth_dynamic_params->enable_negotiate_port =
      command_line->HasSwitch(electron::switches::kEnableAuthNegotiatePort);
  auth_dynamic_params->ntlm_v2_enabled =
      !command_line->HasSwitch(electron::switches::kDisableNTLMv2);
  content::GetNetworkService()->ConfigureHttpAuthPrefs(
      std::move(auth_dynamic_params));
}

void Session::SetUserAgent(const std::string& user_agent,
                           gin::Arguments* args) {
  browser_context_->SetUserAgent(user_agent);
  auto* network_context =
      browser_context_->GetDefaultStoragePartition()->GetNetworkContext();
  network_context->SetUserAgent(user_agent);

  std::string accept_lang;
  if (args->GetNext(&accept_lang)) {
    network_context->SetAcceptLanguage(
        net::HttpUtil::GenerateAcceptLanguageHeader(accept_lang));
  }
}

std::string Session::GetUserAgent() {
  return browser_context_->GetUserAgent();
}

void Session::SetSSLConfig(network::mojom::SSLConfigPtr config) {
  browser_context_->SetSSLConfig(std::move(config));
}

bool Session::IsPersistent() {
  return !browser_context_->IsOffTheRecord();
}

v8::Local<v8::Promise> Session::GetBlobData(v8::Isolate* isolate,
                                            const std::string& uuid) {
  if (DataPipeHolder* holder = DataPipeHolder::From(isolate, uuid)) {
    return holder->ReadAll(isolate);
  }

  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  promise.RejectWithErrorMessage("Could not get blob data handle");
  return promise.GetHandle();
}

void Session::DownloadURL(const GURL& url, gin::Arguments* args) {
  std::map<std::string, std::string> headers;
  gin_helper::Dictionary options;
  if (args->GetNext(&options)) {
    if (options.Has("headers") && !options.Get("headers", &headers)) {
      args->ThrowTypeError("Invalid value for headers - must be an object");
      return;
    }
  }

  auto download_params = std::make_unique<download::DownloadUrlParameters>(
      url, MISSING_TRAFFIC_ANNOTATION);

  for (const auto& [name, value] : headers) {
    download_params->add_request_header(name, value);
  }

  auto* download_manager = browser_context()->GetDownloadManager();
  download_manager->DownloadUrl(std::move(download_params));
}

void Session::CreateInterruptedDownload(const gin_helper::Dictionary& options) {
  int64_t offset = 0, length = 0;
  double start_time = base::Time::Now().InSecondsFSinceUnixEpoch();
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
  if (path.empty() || url_chain.empty() || length == 0) {
    isolate_->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate_, "Must pass non-empty path, urlChain and length.")));
    return;
  }
  if (offset >= length) {
    isolate_->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate_, "Must pass an offset value less than length.")));
    return;
  }
  auto* download_manager = browser_context()->GetDownloadManager();
  download_manager->GetNextId(base::BindRepeating(
      &DownloadIdCallback, download_manager, path, url_chain, mime_type, offset,
      length, last_modified, etag,
      base::Time::FromSecondsSinceUnixEpoch(start_time)));
}

std::string Session::RegisterPreloadScript(
    gin_helper::ErrorThrower thrower,
    const PreloadScript& new_preload_script) {
  auto* prefs = SessionPreferences::FromBrowserContext(browser_context());
  DCHECK(prefs);

  auto& preload_scripts = prefs->preload_scripts();

  auto it = std::find_if(preload_scripts.begin(), preload_scripts.end(),
                         [&new_preload_script](const PreloadScript& script) {
                           return script.id == new_preload_script.id;
                         });

  if (it != preload_scripts.end()) {
    thrower.ThrowError(
        absl::StrFormat("Cannot register preload script with existing ID '%s'",
                        new_preload_script.id));
    return "";
  }

  if (!new_preload_script.file_path.IsAbsolute()) {
    // Deprecated preload scripts logged error without throwing.
    if (new_preload_script.deprecated) {
      LOG(ERROR) << "preload script must have absolute path: "
                 << new_preload_script.file_path;
    } else {
      thrower.ThrowError(
          absl::StrFormat("Preload script must have absolute path: %s",
                          new_preload_script.file_path.value()));
      return "";
    }
  }

  preload_scripts.push_back(new_preload_script);
  return new_preload_script.id;
}

void Session::UnregisterPreloadScript(gin_helper::ErrorThrower thrower,
                                      const std::string& script_id) {
  auto* prefs = SessionPreferences::FromBrowserContext(browser_context());
  DCHECK(prefs);

  auto& preload_scripts = prefs->preload_scripts();

  // Find the preload script by its ID
  auto it = std::find_if(preload_scripts.begin(), preload_scripts.end(),
                         [&script_id](const PreloadScript& script) {
                           return script.id == script_id;
                         });

  // If the script is found, erase it from the vector
  if (it != preload_scripts.end()) {
    preload_scripts.erase(it);
    return;
  }

  // If the script is not found, throw an error
  thrower.ThrowError(absl::StrFormat(
      "Cannot unregister preload script with non-existing ID '%s'", script_id));
}

std::vector<PreloadScript> Session::GetPreloadScripts() const {
  auto* prefs = SessionPreferences::FromBrowserContext(browser_context());
  DCHECK(prefs);
  return prefs->preload_scripts();
}

/**
 * Exposes the network service's ClearSharedDictionaryCacheForIsolationKey
 * method, allowing clearing the Shared Dictionary cache for a given isolation
 * key. Details about the feature available at
 * https://developer.chrome.com/blog/shared-dictionary-compression
 */
v8::Local<v8::Promise> Session::ClearSharedDictionaryCacheForIsolationKey(
    const gin_helper::Dictionary& options) {
  gin_helper::Promise<void> promise(isolate_);
  auto handle = promise.GetHandle();

  GURL frame_origin_url, top_frame_site_url;
  if (!options.Get("frameOrigin", &frame_origin_url) ||
      !options.Get("topFrameSite", &top_frame_site_url)) {
    promise.RejectWithErrorMessage(
        "Must provide frameOrigin and topFrameSite strings to "
        "`clearSharedDictionaryCacheForIsolationKey`");
    return handle;
  }

  if (!frame_origin_url.is_valid() || !top_frame_site_url.is_valid()) {
    promise.RejectWithErrorMessage(
        "Invalid URLs provided for frameOrigin or topFrameSite");
    return handle;
  }

  url::Origin frame_origin = url::Origin::Create(frame_origin_url);
  net::SchemefulSite top_frame_site(top_frame_site_url);
  net::SharedDictionaryIsolationKey isolation_key(frame_origin, top_frame_site);

  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->ClearSharedDictionaryCacheForIsolationKey(
          isolation_key,
          base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                         std::move(promise)));

  return handle;
}

/**
 * Exposes the network service's ClearSharedDictionaryCache
 * method, allowing clearing the Shared Dictionary cache.
 * https://developer.chrome.com/blog/shared-dictionary-compression
 */
v8::Local<v8::Promise> Session::ClearSharedDictionaryCache() {
  gin_helper::Promise<void> promise(isolate_);
  auto handle = promise.GetHandle();

  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->ClearSharedDictionaryCache(
          base::Time(), base::Time::Max(),
          nullptr /*mojom::ClearDataFilterPtr*/,
          base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                         std::move(promise)));

  return handle;
}

/**
 * Exposes the network service's GetSharedDictionaryInfo method, allowing
 * inspection of Shared Dictionary information. Details about the feature
 * available at https://developer.chrome.com/blog/shared-dictionary-compression
 */
v8::Local<v8::Promise> Session::GetSharedDictionaryInfo(
    const gin_helper::Dictionary& options) {
  gin_helper::Promise<std::vector<gin_helper::Dictionary>> promise(isolate_);
  auto handle = promise.GetHandle();

  GURL frame_origin_url, top_frame_site_url;
  if (!options.Get("frameOrigin", &frame_origin_url) ||
      !options.Get("topFrameSite", &top_frame_site_url)) {
    promise.RejectWithErrorMessage(
        "Must provide frameOrigin and topFrameSite strings");
    return handle;
  }

  if (!frame_origin_url.is_valid() || !top_frame_site_url.is_valid()) {
    promise.RejectWithErrorMessage(
        "Invalid URLs provided for frameOrigin or topFrameSite");
    return handle;
  }

  url::Origin frame_origin = url::Origin::Create(frame_origin_url);
  net::SchemefulSite top_frame_site(top_frame_site_url);
  net::SharedDictionaryIsolationKey isolation_key(frame_origin, top_frame_site);

  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->GetSharedDictionaryInfo(
          isolation_key,
          base::BindOnce(
              [](gin_helper::Promise<std::vector<gin_helper::Dictionary>>
                     promise,
                 std::vector<network::mojom::SharedDictionaryInfoPtr> info) {
                v8::Isolate* isolate = promise.isolate();
                v8::HandleScope handle_scope(isolate);

                std::vector<gin_helper::Dictionary> result;
                result.reserve(info.size());

                for (const auto& item : info) {
                  gin_helper::Dictionary dict =
                      gin_helper::Dictionary::CreateEmpty(isolate);
                  dict.Set("match", item->match);

                  // Convert RequestDestination enum values to strings
                  std::vector<std::string> destinations;
                  for (const auto& dest : item->match_dest) {
                    destinations.push_back(
                        network::RequestDestinationToString(dest));
                  }
                  dict.Set("matchDestinations", destinations);
                  dict.Set("id", item->id);
                  dict.Set("dictionaryUrl", item->dictionary_url.spec());
                  dict.Set("lastFetchTime", item->last_fetch_time);
                  dict.Set("responseTime", item->response_time);
                  dict.Set("expirationDuration",
                           item->expiration.InMillisecondsF());
                  dict.Set("lastUsedTime", item->last_used_time);
                  dict.Set("size", item->size);
                  dict.Set("hash", net::HashValue(item->hash).ToString());

                  result.push_back(dict);
                }

                promise.Resolve(result);
              },
              std::move(promise)));

  return handle;
}

/**
 * Exposes the network service's GetSharedDictionaryUsageInfo method, allowing
 * inspection of Shared Dictionary information. Details about the feature
 * available at https://developer.chrome.com/blog/shared-dictionary-compression
 */
v8::Local<v8::Promise> Session::GetSharedDictionaryUsageInfo() {
  gin_helper::Promise<std::vector<gin_helper::Dictionary>> promise(isolate_);
  auto handle = promise.GetHandle();

  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->GetSharedDictionaryUsageInfo(base::BindOnce(
          [](gin_helper::Promise<std::vector<gin_helper::Dictionary>> promise,
             const std::vector<net::SharedDictionaryUsageInfo>& info) {
            v8::Isolate* isolate = promise.isolate();
            v8::HandleScope handle_scope(isolate);

            std::vector<gin_helper::Dictionary> result;
            result.reserve(info.size());

            for (const auto& item : info) {
              gin_helper::Dictionary dict =
                  gin_helper::Dictionary::CreateEmpty(isolate);
              dict.Set("frameOrigin",
                       item.isolation_key.frame_origin().Serialize());
              dict.Set("topFrameSite",
                       item.isolation_key.top_frame_site().Serialize());
              dict.Set("totalSizeBytes", item.total_size_bytes);
              result.push_back(dict);
            }

            promise.Resolve(result);
          },
          std::move(promise)));

  return handle;
}

v8::Local<v8::Value> Session::Cookies(v8::Isolate* isolate) {
  if (cookies_.IsEmptyThreadSafe()) {
    auto handle = Cookies::Create(isolate, browser_context());
    cookies_.Reset(isolate, handle.ToV8());
  }
  return cookies_.Get(isolate);
}

v8::Local<v8::Value> Session::Extensions(v8::Isolate* isolate) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  if (extensions_.IsEmptyThreadSafe()) {
    v8::Local<v8::Value> handle;
    handle = Extensions::Create(isolate, browser_context()).ToV8();
    extensions_.Reset(isolate, handle);
  }
#endif
  return extensions_.Get(isolate);
}

v8::Local<v8::Value> Session::Protocol(v8::Isolate* isolate) {
  return protocol_.Get(isolate);
}

v8::Local<v8::Value> Session::ServiceWorkerContext(v8::Isolate* isolate) {
  if (service_worker_context_.IsEmptyThreadSafe()) {
    v8::Local<v8::Value> handle;
    handle = ServiceWorkerContext::Create(isolate, browser_context()).ToV8();
    service_worker_context_.Reset(isolate, handle);
  }
  return service_worker_context_.Get(isolate);
}

WebRequest* Session::WebRequest(v8::Isolate* isolate) {
  if (!web_request_)
    web_request_ = WebRequest::Create(isolate, base::PassKey<Session>{});
  return web_request_;
}

NetLog* Session::NetLog(v8::Isolate* isolate) {
  if (!net_log_) {
    net_log_ = NetLog::Create(isolate, browser_context());
  }
  return net_log_;
}

static void StartPreconnectOnUI(ElectronBrowserContext* browser_context,
                                const GURL& url,
                                int num_sockets_to_preconnect) {
  url::Origin origin = url::Origin::Create(url);
  std::vector<content::PreconnectRequest> requests = {
      {url::Origin::Create(url), num_sockets_to_preconnect,
       net::NetworkAnonymizationKey::CreateSameSite(
           net::SchemefulSite(origin))}};
  browser_context->GetPreconnectManager()->Start(
      url, requests, predictors::kLoadingPredictorPreconnectTrafficAnnotation);
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
          absl::StrFormat("numSocketsToPreconnect is outside range [%d,%d]",
                          kMinSocketsToPreconnect, kMaxSocketsToPreconnect));
      return;
    }
  }

  DCHECK_GT(num_sockets_to_preconnect, 0);
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&StartPreconnectOnUI, base::Unretained(browser_context()),
                     url, num_sockets_to_preconnect));
}

v8::Local<v8::Promise> Session::CloseAllConnections() {
  gin_helper::Promise<void> promise(isolate_);
  auto handle = promise.GetHandle();

  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->CloseAllConnections(base::BindOnce(
          gin_helper::Promise<void>::ResolvePromise, std::move(promise)));

  return handle;
}

v8::Local<v8::Value> Session::GetPath(v8::Isolate* isolate) {
  if (browser_context_->IsOffTheRecord()) {
    return v8::Null(isolate);
  }
  return gin::ConvertToV8(isolate, browser_context_->GetPath());
}

void Session::SetCodeCachePath(gin::Arguments* args) {
  base::FilePath code_cache_path;
  auto* storage_partition = browser_context_->GetDefaultStoragePartition();
  auto* code_cache_context = storage_partition->GetGeneratedCodeCacheContext();
  if (code_cache_context) {
    if (!args->GetNext(&code_cache_path) || !code_cache_path.IsAbsolute()) {
      args->ThrowTypeError(
          "Absolute path must be provided to store code cache.");
      return;
    }
    code_cache_context->Initialize(
        code_cache_path, 0 /* allows disk_cache to choose the size */);
  }
}

v8::Local<v8::Promise> Session::ClearCodeCaches(
    const gin_helper::Dictionary& options) {
  auto* isolate = JavascriptEnvironment::GetIsolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  std::set<GURL> url_list;
  base::RepeatingCallback<bool(const GURL&)> url_matcher = base::NullCallback();
  if (options.Get("urls", &url_list) && !url_list.empty()) {
    url_matcher = base::BindRepeating(
        [](const std::set<GURL>& url_list, const GURL& url) {
          return url_list.contains(url);
        },
        url_list);
  }

  browser_context_->GetDefaultStoragePartition()->ClearCodeCaches(
      base::Time(), base::Time::Max(), url_matcher,
      base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                     std::move(promise)));

  return handle;
}

v8::Local<v8::Value> Session::ClearData(gin::Arguments* const args) {
  v8::Isolate* const isolate = JavascriptEnvironment::GetIsolate();

  BrowsingDataRemover::DataType data_type_mask = kClearDataTypeAll;
  std::vector<url::Origin> origins;
  BrowsingDataFilterBuilder::OriginMatchingMode origin_matching_mode =
      BrowsingDataFilterBuilder::OriginMatchingMode::kThirdPartiesIncluded;
  BrowsingDataFilterBuilder::Mode filter_mode =
      BrowsingDataFilterBuilder::Mode::kPreserve;

  if (gin_helper::Dictionary options; args->GetNext(&options)) {
    if (std::vector<std::string> data_types;
        options.Get("dataTypes", &data_types)) {
      data_type_mask = GetDataTypeMask(data_types);
    }

    if (bool avoid_closing_connections;
        options.Get("avoidClosingConnections", &avoid_closing_connections) &&
        avoid_closing_connections) {
      data_type_mask |=
          BrowsingDataRemover::DATA_TYPE_AVOID_CLOSING_CONNECTIONS;
    }

    std::vector<GURL> origin_urls;
    {
      bool has_origins_key = options.Get("origins", &origin_urls);
      std::vector<GURL> exclude_origin_urls;
      bool has_exclude_origins_key =
          options.Get("excludeOrigins", &exclude_origin_urls);

      if (has_origins_key && has_exclude_origins_key) {
        args->ThrowTypeError(
            "Cannot provide both 'origins' and 'excludeOrigins'");
        return v8::Undefined(isolate);
      }

      if (has_origins_key) {
        filter_mode = BrowsingDataFilterBuilder::Mode::kDelete;
      } else if (has_exclude_origins_key) {
        origin_urls = std::move(exclude_origin_urls);
      }
    }

    if (!origin_urls.empty()) {
      origins.reserve(origin_urls.size());
      for (const GURL& origin_url : origin_urls) {
        auto origin = url::Origin::Create(origin_url);

        // Opaque origins cannot be used with this API
        if (origin.opaque()) {
          args->ThrowTypeError(absl::StrFormat(
              "Invalid origin: '%s'", origin_url.possibly_invalid_spec()));
          return v8::Undefined(isolate);
        }

        origins.push_back(std::move(origin));
      }
    }

    if (std::string origin_matching_mode_string;
        options.Get("originMatchingMode", &origin_matching_mode_string)) {
      if (origin_matching_mode_string == "third-parties-included") {
        origin_matching_mode = BrowsingDataFilterBuilder::OriginMatchingMode::
            kThirdPartiesIncluded;
      } else if (origin_matching_mode_string == "origin-in-all-contexts") {
        origin_matching_mode =
            BrowsingDataFilterBuilder::OriginMatchingMode::kOriginInAllContexts;
      }
    }
  }

  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> promise_handle = promise.GetHandle();

  BrowsingDataRemover* remover = browser_context_->GetBrowsingDataRemover();
  ClearDataTask::Run(remover, std::move(promise), data_type_mask,
                     std::move(origins), filter_mode, origin_matching_mode);

  return promise_handle;
}

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
base::Value Session::GetSpellCheckerLanguages() {
  return browser_context_->prefs()
      ->GetValue(spellcheck::prefs::kSpellCheckDictionaries)
      .Clone();
}

void Session::SetSpellCheckerLanguages(
    gin_helper::ErrorThrower thrower,
    const std::vector<std::string>& languages) {
#if !BUILDFLAG(IS_MAC)
  base::ListValue language_codes;
  for (const std::string& lang : languages) {
    std::string code = spellcheck::GetCorrespondingSpellCheckLanguage(lang);
    if (code.empty()) {
      thrower.ThrowError("Invalid language code provided: \"" + lang +
                         "\" is not a valid language code");
      return;
    }
    language_codes.Append(code);
  }
  browser_context_->prefs()->Set(spellcheck::prefs::kSpellCheckDictionaries,
                                 base::Value(std::move(language_codes)));
  // Enable spellcheck if > 0 languages, disable if no languages set
  browser_context_->prefs()->SetBoolean(spellcheck::prefs::kSpellCheckEnable,
                                        !languages.empty());
#endif
}

void SetSpellCheckerDictionaryDownloadURL(gin_helper::ErrorThrower thrower,
                                          const GURL& url) {
#if !BUILDFLAG(IS_MAC)
  if (!url.is_valid()) {
    thrower.ThrowError(
        "The URL you provided to setSpellCheckerDictionaryDownloadURL is not a "
        "valid URL");
    return;
  }
  SpellcheckHunspellDictionary::SetBaseDownloadURL(url);
#endif
}

v8::Local<v8::Promise> Session::ListWordsInSpellCheckerDictionary() {
  gin_helper::Promise<std::set<std::string>> promise(isolate_);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  SpellcheckService* spellcheck =
      SpellcheckServiceFactory::GetForContext(browser_context());

  if (!spellcheck) {
    promise.RejectWithErrorMessage(
        "Spellcheck in unexpected state: failed to load custom dictionary.");
    return handle;
  }

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
      SpellcheckServiceFactory::GetForContext(browser_context());
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
      SpellcheckServiceFactory::GetForContext(browser_context());
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

void Session::SetSpellCheckerEnabled(bool b) {
  browser_context_->prefs()->SetBoolean(spellcheck::prefs::kSpellCheckEnable,
                                        b);
}

bool Session::IsSpellCheckerEnabled() const {
  return browser_context_->prefs()->GetBoolean(
      spellcheck::prefs::kSpellCheckEnable);
}

#endif  // BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)

// static
gin::WeakCell<Session>* Session::FromBrowserContext(
    content::BrowserContext* context) {
  auto* data =
      static_cast<UserDataLink*>(context->GetUserData(kElectronApiSessionKey));
  if (data && data->session)
    return data->session.Get();
  return nullptr;
}

// static
Session* Session::FromOrCreate(v8::Isolate* isolate,
                               content::BrowserContext* context) {
  if (ElectronBrowserContext::IsValidContext(context))
    return FromOrCreate(isolate, static_cast<ElectronBrowserContext*>(context));
  DCHECK(false);
  return {};
}

// static
Session* Session::FromOrCreate(v8::Isolate* isolate,
                               ElectronBrowserContext* browser_context) {
  gin::WeakCell<Session>* existing = FromBrowserContext(browser_context);
  if (existing && existing->Get()) {
    return existing->Get();
  }

  auto* session = cppgc::MakeGarbageCollected<Session>(
      isolate->GetCppHeap()->GetAllocationHandle(), isolate, browser_context);

  v8::TryCatch try_catch(isolate);
  gin_helper::CallMethod(isolate, session, "_init");
  if (try_catch.HasCaught()) {
    node::errors::TriggerUncaughtException(isolate, try_catch);
    return nullptr;
  }

  v8::Local<v8::Object> wrapper;
  if (!session->GetWrapper(isolate).ToLocal(&wrapper)) {
    return nullptr;
  }
  App::Get()->EmitWithoutEvent("session-created", wrapper);

  return session;
}

// static
Session* Session::FromPartition(v8::Isolate* isolate,
                                const std::string& partition,
                                base::DictValue options) {
  ElectronBrowserContext* browser_context;
  if (partition.empty()) {
    browser_context =
        ElectronBrowserContext::GetDefaultBrowserContext(std::move(options));
  } else if (partition.starts_with(kPersistPrefix)) {
    std::string name = partition.substr(8);
    browser_context =
        ElectronBrowserContext::From(name, false, std::move(options));
  } else {
    browser_context =
        ElectronBrowserContext::From(partition, true, std::move(options));
  }
  return FromOrCreate(isolate, browser_context);
}

// static
Session* Session::FromPath(gin::Arguments* args,
                           const base::FilePath& path,
                           base::DictValue options) {
  ElectronBrowserContext* browser_context;

  if (path.empty()) {
    args->ThrowTypeError("An empty path was specified");
    return nullptr;
  }
  if (!path.IsAbsolute()) {
    args->ThrowTypeError("An absolute path was not provided");
    return nullptr;
  }

  browser_context =
      ElectronBrowserContext::FromPath(std::move(path), std::move(options));

  return FromOrCreate(args->isolate(), browser_context);
}

// static
void Session::New() {
  gin_helper::ErrorThrower(JavascriptEnvironment::GetIsolate())
      .ThrowError("Session objects cannot be created with 'new'");
}

void Session::FillObjectTemplate(v8::Isolate* isolate,
                                 v8::Local<v8::ObjectTemplate> templ) {
  gin::ObjectTemplateBuilder(isolate, GetClassName(), templ)
      .SetMethod("resolveHost", &Session::ResolveHost)
      .SetMethod("resolveProxy", &Session::ResolveProxy)
      .SetMethod("getCacheSize", &Session::GetCacheSize)
      .SetMethod("clearCache", &Session::ClearCache)
      .SetMethod("clearStorageData", &Session::ClearStorageData)
      .SetMethod("flushStorageData", &Session::FlushStorageData)
      .SetMethod("setProxy", &Session::SetProxy)
      .SetMethod("forceReloadProxyConfig", &Session::ForceReloadProxyConfig)
      .SetMethod("setDownloadPath", &Session::SetDownloadPath)
      .SetMethod("enableNetworkEmulation", &Session::EnableNetworkEmulation)
      .SetMethod("disableNetworkEmulation", &Session::DisableNetworkEmulation)
      .SetMethod("setCertificateVerifyProc", &Session::SetCertVerifyProc)
      .SetMethod("setPermissionRequestHandler",
                 &Session::SetPermissionRequestHandler)
      .SetMethod("setPermissionCheckHandler",
                 &Session::SetPermissionCheckHandler)
      .SetMethod("_setDisplayMediaRequestHandler",
                 &Session::SetDisplayMediaRequestHandler)
      .SetMethod("setDevicePermissionHandler",
                 &Session::SetDevicePermissionHandler)
      .SetMethod("setUSBProtectedClassesHandler",
                 &Session::SetUSBProtectedClassesHandler)
      .SetMethod("setBluetoothPairingHandler",
                 &Session::SetBluetoothPairingHandler)
      .SetMethod("clearHostResolverCache", &Session::ClearHostResolverCache)
      .SetMethod("clearAuthCache", &Session::ClearAuthCache)
      .SetMethod("allowNTLMCredentialsForDomains",
                 &Session::AllowNTLMCredentialsForDomains)
      .SetMethod("isPersistent", &Session::IsPersistent)
      .SetMethod("setUserAgent", &Session::SetUserAgent)
      .SetMethod("getUserAgent", &Session::GetUserAgent)
      .SetMethod("setSSLConfig", &Session::SetSSLConfig)
      .SetMethod("getBlobData", &Session::GetBlobData)
      .SetMethod("downloadURL", &Session::DownloadURL)
      .SetMethod("createInterruptedDownload",
                 &Session::CreateInterruptedDownload)
      .SetMethod("registerPreloadScript", &Session::RegisterPreloadScript)
      .SetMethod("unregisterPreloadScript", &Session::UnregisterPreloadScript)
      .SetMethod("getPreloadScripts", &Session::GetPreloadScripts)
      .SetMethod("getSharedDictionaryUsageInfo",
                 &Session::GetSharedDictionaryUsageInfo)
      .SetMethod("getSharedDictionaryInfo", &Session::GetSharedDictionaryInfo)
      .SetMethod("clearSharedDictionaryCache",
                 &Session::ClearSharedDictionaryCache)
      .SetMethod("clearSharedDictionaryCacheForIsolationKey",
                 &Session::ClearSharedDictionaryCacheForIsolationKey)
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
      .SetMethod("setSpellCheckerEnabled", &Session::SetSpellCheckerEnabled)
      .SetMethod("isSpellCheckerEnabled", &Session::IsSpellCheckerEnabled)
      .SetProperty("spellCheckerEnabled", &Session::IsSpellCheckerEnabled,
                   &Session::SetSpellCheckerEnabled)
#endif
      .SetMethod("preconnect", &Session::Preconnect)
      .SetMethod("closeAllConnections", &Session::CloseAllConnections)
      .SetMethod("getStoragePath", &Session::GetPath)
      .SetMethod("setCodeCachePath", &Session::SetCodeCachePath)
      .SetMethod("clearCodeCaches", &Session::ClearCodeCaches)
      .SetMethod("clearData", &Session::ClearData)
      .SetProperty("cookies", &Session::Cookies)
      .SetProperty("extensions", &Session::Extensions)
      .SetProperty("netLog", &Session::NetLog)
      .SetProperty("protocol", &Session::Protocol)
      .SetProperty("serviceWorkers", &Session::ServiceWorkerContext)
      .SetProperty("webRequest", &Session::WebRequest)
      .SetProperty("storagePath", &Session::GetPath)
      .Build();
}

void Session::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<Session>::Trace(visitor);
  visitor->Trace(cookies_);
  visitor->Trace(extensions_);
  visitor->Trace(protocol_);
  visitor->Trace(net_log_);
  visitor->Trace(service_worker_context_);
  visitor->Trace(web_request_);
  visitor->Trace(weak_factory_);
}

const gin::WrapperInfo* Session::wrapper_info() const {
  return &kWrapperInfo;
}

const char* Session::GetHumanReadableName() const {
  return "Electron / Session";
}

void Session::OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) {
  gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
  data->RemoveDisposeObserver(this);
  Dispose();
  weak_factory_.Invalidate();
  keep_alive_.Clear();
}

}  // namespace electron::api

namespace {

using electron::api::Session;

Session* FromPartition(const std::string& partition, gin::Arguments* args) {
  if (!electron::Browser::Get()->is_ready()) {
    args->ThrowTypeError("Session can only be received when app is ready");
    return {};
  }
  base::DictValue options;
  args->GetNext(&options);
  return Session::FromPartition(args->isolate(), partition, std::move(options));
}

Session* FromPath(const base::FilePath& path, gin::Arguments* args) {
  if (!electron::Browser::Get()->is_ready()) {
    args->ThrowTypeError("Session can only be received when app is ready");
    return {};
  }
  base::DictValue options;
  args->GetNext(&options);
  return Session::FromPath(args, path, std::move(options));
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("Session",
           Session::GetConstructor(isolate, context, &Session::kWrapperInfo));
  dict.SetMethod("fromPartition", &FromPartition);
  dict.SetMethod("fromPath", &FromPath);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_session, Initialize)
