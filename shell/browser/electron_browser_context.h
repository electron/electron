// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_BROWSER_CONTEXT_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_BROWSER_CONTEXT_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "base/files/file_path.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/media_stream_request.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/ssl_config.mojom.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

class PrefService;
class ValueMapPrefStore;

namespace content {
class PreconnectManager;
}  // namespace content

namespace gin {
class Arguments;
}

namespace network {
class SharedURLLoaderFactory;
}

namespace storage {
class SpecialStoragePolicy;
}

namespace electron {

class CookieChangeNotifier;
class ElectronDownloadManagerDelegate;
class ElectronPermissionManager;
class ElectronPreconnectManagerDelegate;
class MediaDeviceIDSalt;
class ProtocolRegistry;
class ResolveProxyHelper;
class WebViewManager;

using DisplayMediaResponseCallbackJs =
    base::OnceCallback<void(gin::Arguments* args)>;
using DisplayMediaRequestHandler =
    base::RepeatingCallback<void(const content::MediaStreamRequest&,
                                 DisplayMediaResponseCallbackJs)>;
class ElectronBrowserContext : public content::BrowserContext {
 public:
  // disable copy
  ElectronBrowserContext(const ElectronBrowserContext&) = delete;
  ElectronBrowserContext& operator=(const ElectronBrowserContext&) = delete;

  [[nodiscard]] static std::vector<ElectronBrowserContext*> BrowserContexts();

  [[nodiscard]] static bool IsValidContext(const void* context);

  // Get or create the default BrowserContext.
  static ElectronBrowserContext* GetDefaultBrowserContext(
      base::DictValue options = {});

  // Get or create the BrowserContext according to its |partition| and
  // |in_memory|. The |options| will be passed to constructor when there is no
  // existing BrowserContext.
  static ElectronBrowserContext* From(const std::string& partition,
                                      bool in_memory,
                                      base::DictValue options = {});

  // Get or create the BrowserContext using the |path|.
  // The |options| will be passed to constructor when there is no
  // existing BrowserContext.
  static ElectronBrowserContext* FromPath(const base::FilePath& path,
                                          base::DictValue options = {});

  static void DestroyAllContexts();

  void SetUserAgent(const std::string& user_agent);
  std::string GetUserAgent() const;
  bool can_use_http_cache() const { return use_cache_; }
  int max_cache_size() const { return max_cache_size_; }
  ResolveProxyHelper* GetResolveProxyHelper();
  content::PreconnectManager* GetPreconnectManager();
  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory();

  std::string GetMediaDeviceIDSalt();

  // content::BrowserContext:
  base::FilePath GetPath() const override;
  bool IsOffTheRecord() override;
  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  content::BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;
  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate()
      override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  content::PlatformNotificationService* GetPlatformNotificationService()
      override;
  content::PermissionControllerDelegate* GetPermissionControllerDelegate()
      override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  content::ClientHintsControllerDelegate* GetClientHintsControllerDelegate()
      override;
  content::StorageNotificationService* GetStorageNotificationService() override;
  content::ReduceAcceptLanguageControllerDelegate*
  GetReduceAcceptLanguageControllerDelegate() override;
  content::FileSystemAccessPermissionContext*
  GetFileSystemAccessPermissionContext() override;

  CookieChangeNotifier* cookie_change_notifier() const {
    return cookie_change_notifier_.get();
  }
  PrefService* prefs() const { return prefs_.get(); }
  ValueMapPrefStore* in_memory_pref_store() const {
    return in_memory_pref_store_.get();
  }

  ProtocolRegistry* protocol_registry() const {
    return protocol_registry_.get();
  }

  void SetSSLConfig(network::mojom::SSLConfigPtr config);
  network::mojom::SSLConfigPtr GetSSLConfig();
  void SetSSLConfigClient(mojo::Remote<network::mojom::SSLConfigClient> client);

  bool ChooseDisplayMediaDevice(const content::MediaStreamRequest& request,
                                content::MediaResponseCallback callback);
  void SetDisplayMediaRequestHandler(DisplayMediaRequestHandler handler);

  ~ElectronBrowserContext() override;

  // Grants |origin| access to |device|.
  // To be used in place of ObjectPermissionContextBase::GrantObjectPermission.
  void GrantDevicePermission(const url::Origin& origin,
                             const base::Value& device,
                             blink::PermissionType permissionType);

  // Revokes |origin| access to |device|.
  // To be used in place of ObjectPermissionContextBase::RevokeObjectPermission.
  void RevokeDevicePermission(const url::Origin& origin,
                              const base::Value& device,
                              blink::PermissionType permission_type);

  // Returns the list of devices that |origin| has been granted permission to
  // access. To be used in place of
  // ObjectPermissionContextBase::GetGrantedObjects.
  bool CheckDevicePermission(const url::Origin& origin,
                             const base::Value& device,
                             blink::PermissionType permissionType);

 private:
  using DevicePermissionMap = std::map<
      blink::PermissionType,
      std::map<url::Origin, std::vector<std::unique_ptr<base::Value>>>>;

  using PartitionOrPath =
      std::variant<std::reference_wrapper<const std::string>,
                   std::reference_wrapper<const base::FilePath>>;

  ElectronBrowserContext(const PartitionOrPath partition_location,
                         bool in_memory,
                         base::DictValue options);

  ElectronBrowserContext(base::FilePath partition, base::DictValue options);

  static void DisplayMediaDeviceChosen(
      const content::MediaStreamRequest& request,
      content::MediaResponseCallback callback,
      gin::Arguments* args);

  // Initialize pref registry.
  void InitPrefs();

  scoped_refptr<ValueMapPrefStore> in_memory_pref_store_;
  std::unique_ptr<CookieChangeNotifier> cookie_change_notifier_;
  std::unique_ptr<PrefService> prefs_;
  std::unique_ptr<ElectronDownloadManagerDelegate> download_manager_delegate_;
  std::unique_ptr<WebViewManager> guest_manager_;
  std::unique_ptr<ElectronPermissionManager> permission_manager_;
  std::unique_ptr<MediaDeviceIDSalt> media_device_id_salt_;
  scoped_refptr<ResolveProxyHelper> resolve_proxy_helper_;
  scoped_refptr<storage::SpecialStoragePolicy> storage_policy_;
  std::unique_ptr<content::PreconnectManager> preconnect_manager_;
  std::unique_ptr<ElectronPreconnectManagerDelegate>
      preconnect_manager_delegate_;
  std::unique_ptr<ProtocolRegistry> protocol_registry_;

  std::optional<std::string> user_agent_;
  base::FilePath path_;
  bool in_memory_ = false;
  bool use_cache_ = true;
  int max_cache_size_ = 0;

  // Shared URLLoaderFactory.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  network::mojom::SSLConfigPtr ssl_config_;
  mojo::Remote<network::mojom::SSLConfigClient> ssl_config_client_;

  DisplayMediaRequestHandler display_media_request_handler_;

  // In-memory cache that holds objects that have been granted permissions.
  DevicePermissionMap granted_devices_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_BROWSER_CONTEXT_H_
