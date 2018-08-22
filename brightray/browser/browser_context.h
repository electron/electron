// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_BROWSER_CONTEXT_H_
#define BRIGHTRAY_BROWSER_BROWSER_CONTEXT_H_

#include <map>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "brightray/browser/media/media_device_id_salt.h"
#include "brightray/browser/url_request_context_getter.h"
#include "content/public/browser/browser_context.h"

class PrefRegistrySimple;
class PrefService;

namespace storage {
class SpecialStoragePolicy;
}

namespace brightray {

class BrowserContext;

struct BrowserContextDeleter {
  static void Destruct(const BrowserContext* browser_context);
};

class BrowserContext
    : public base::RefCountedThreadSafe<BrowserContext, BrowserContextDeleter>,
      public content::BrowserContext {
 public:
  // Get the BrowserContext according to its |partition| and |in_memory|,
  // empty pointer when be returned when there is no matching BrowserContext.
  static scoped_refptr<BrowserContext> Get(const std::string& partition,
                                           bool in_memory);

  base::WeakPtr<BrowserContext> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  // Get the request context, if there is no one, create it.
  URLRequestContextGetter* GetRequestContext();

  // content::BrowserContext:
  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override;
  bool IsOffTheRecord() const override;
  content::ResourceContext* GetResourceContext() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  content::BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;
  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate()
      override;
  net::URLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateMediaRequestContext() override;
  net::URLRequestContextGetter* CreateMediaRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory) override;
  std::string GetMediaDeviceIDSalt() override;
  base::FilePath GetPath() const override;

  void InitPrefs();
  PrefService* prefs() { return prefs_.get(); }

  virtual std::string GetUserAgent() const = 0;
  virtual void OnMainRequestContextCreated(URLRequestContextGetter* getter) {}

 protected:
  BrowserContext(const std::string& partition, bool in_memory);
  ~BrowserContext() override;

  // Subclasses should override this to register custom preferences.
  virtual void RegisterPrefs(PrefRegistrySimple* pref_registry) {}

 private:
  friend class base::RefCountedThreadSafe<BrowserContext,
                                          BrowserContextDeleter>;
  friend class base::DeleteHelper<BrowserContext>;
  friend struct BrowserContextDeleter;

  void RegisterInternalPrefs(PrefRegistrySimple* pref_registry);
  void OnDestruct() const;

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
      std::map<PartitionKey, base::WeakPtr<brightray::BrowserContext>>;
  static BrowserContextMap browser_context_map_;

  base::FilePath path_;
  bool in_memory_;

  scoped_refptr<storage::SpecialStoragePolicy> storage_policy_;
  std::unique_ptr<PrefService> prefs_;
  std::unique_ptr<MediaDeviceIDSalt> media_device_id_salt_;
  // Self-destructing class responsible for creating URLRequestContextGetter
  // on the UI thread and deletes itself on the IO thread.
  URLRequestContextGetter::Handle* io_handle_;

  base::WeakPtrFactory<BrowserContext> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BrowserContext);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_BROWSER_CONTEXT_H_
