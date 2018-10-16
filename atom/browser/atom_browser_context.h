// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "atom/browser/net/url_request_context_getter.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/memory/weak_ptr.h"
#include "brightray/browser/media/media_device_id_salt.h"
#include "chrome/browser/net/proxy_config_monitor.h"
#include "content/public/browser/browser_context.h"

class PrefRegistrySimple;
class PrefService;
class ValueMapPrefStore;

namespace storage {
class SpecialStoragePolicy;
}

namespace atom {

class AtomBlobReader;
class AtomBrowserContext;
class AtomDownloadManagerDelegate;
class AtomPermissionManager;
class CookieChangeNotifier;
class ResolveProxyHelper;
class SpecialStoragePolicy;
class WebViewManager;

class AtomBrowserContext
    : public base::RefCountedDeleteOnSequence<AtomBrowserContext>,
      public content::BrowserContext {
 public:
  // Get or create the BrowserContext according to its |partition| and
  // |in_memory|. The |options| will be passed to constructor when there is no
  // existing BrowserContext.
  static scoped_refptr<AtomBrowserContext> From(
      const std::string& partition,
      bool in_memory,
      const base::DictionaryValue& options = base::DictionaryValue());

  void SetUserAgent(const std::string& user_agent);
  std::string GetUserAgent() const;
  bool CanUseHttpCache() const;
  int GetMaxCacheSize() const;
  AtomBlobReader* GetBlobReader();
  network::mojom::NetworkContextPtr GetNetworkContext();
  // Get the request context, if there is none, create it.
  net::URLRequestContextGetter* GetRequestContext();
  ResolveProxyHelper* GetResolveProxyHelper();

  // content::BrowserContext:
  base::FilePath GetPath() const override;
  bool IsOffTheRecord() const override;
  content::ResourceContext* GetResourceContext() override;
  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  content::BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;
  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate()
      override;
  net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateMediaRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory) override;
  std::string GetMediaDeviceIDSalt() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  content::PermissionControllerDelegate* GetPermissionControllerDelegate()
      override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  net::URLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateMediaRequestContext() override;

  CookieChangeNotifier* cookie_change_notifier() const {
    return cookie_change_notifier_.get();
  }
  ProxyConfigMonitor* proxy_config_monitor() {
    return proxy_config_monitor_.get();
  }
  PrefService* prefs() const { return prefs_.get(); }
  void set_in_memory_pref_store(ValueMapPrefStore* pref_store) {
    in_memory_pref_store_ = pref_store;
  }
  ValueMapPrefStore* in_memory_pref_store() const {
    return in_memory_pref_store_;
  }
  base::WeakPtr<AtomBrowserContext> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 protected:
  AtomBrowserContext(const std::string& partition,
                     bool in_memory,
                     const base::DictionaryValue& options);
  ~AtomBrowserContext() override;

 private:
  friend class base::RefCountedDeleteOnSequence<AtomBrowserContext>;
  friend class base::DeleteHelper<AtomBrowserContext>;

  // Initialize pref registry.
  void InitPrefs();

  // partition_id => browser_context
  struct PartitionKey {
    std::string partition;
    bool in_memory;

    PartitionKey(const std::string& partition, bool in_memory)
        : partition(partition), in_memory(in_memory) {}

    bool operator<(const PartitionKey& other) const {
      if (partition == other.partition)
        return in_memory < other.in_memory;
      return partition < other.partition;
    }

    bool operator==(const PartitionKey& other) const {
      return (partition == other.partition) && (in_memory == other.in_memory);
    }
  };
  using BrowserContextMap =
      std::map<PartitionKey, base::WeakPtr<AtomBrowserContext>>;
  static BrowserContextMap browser_context_map_;

  // Self-destructing class responsible for creating URLRequestContextGetter
  // on the UI thread and deletes itself on the IO thread.
  URLRequestContextGetter::Handle* io_handle_;
  ValueMapPrefStore* in_memory_pref_store_;

  std::unique_ptr<CookieChangeNotifier> cookie_change_notifier_;
  std::unique_ptr<PrefService> prefs_;
  std::unique_ptr<AtomDownloadManagerDelegate> download_manager_delegate_;
  std::unique_ptr<WebViewManager> guest_manager_;
  std::unique_ptr<AtomPermissionManager> permission_manager_;
  std::unique_ptr<AtomBlobReader> blob_reader_;
  std::unique_ptr<brightray::MediaDeviceIDSalt> media_device_id_salt_;
  scoped_refptr<ResolveProxyHelper> resolve_proxy_helper_;
  scoped_refptr<storage::SpecialStoragePolicy> storage_policy_;

  // Tracks the ProxyConfig to use, and passes any updates to a NetworkContext
  // ProxyConfigClient.
  std::unique_ptr<ProxyConfigMonitor> proxy_config_monitor_;

  std::string user_agent_;
  base::FilePath path_;
  bool in_memory_ = false;
  bool use_cache_ = true;
  int max_cache_size_ = 0;

  base::WeakPtrFactory<AtomBrowserContext> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserContext);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
