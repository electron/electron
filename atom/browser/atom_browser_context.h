// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_

#include <string>
#include <vector>

#include "base/callback_list.h"
#include "brightray/browser/browser_context.h"

namespace atom {

class AtomBlobReader;
class AtomDownloadManagerDelegate;
class AtomNetworkDelegate;
class AtomPermissionManager;
class WebViewManager;
struct CookieDetails;

class AtomBrowserContext : public brightray::BrowserContext {
 public:
  // Get or create the BrowserContext according to its |partition| and
  // |in_memory|. The |options| will be passed to constructor when there is no
  // existing BrowserContext.
  static scoped_refptr<AtomBrowserContext> From(
      const std::string& partition, bool in_memory,
      const base::DictionaryValue& options = base::DictionaryValue());

  void SetUserAgent(const std::string& user_agent);
  // Register callbacks that needs to notified on any cookie store changes.
  std::unique_ptr<base::CallbackList<void(const CookieDetails*)>::Subscription>
  RegisterCookieChangeCallback(
      const base::Callback<void(const CookieDetails*)>& cb);

  // brightray::URLRequestContextGetter::Delegate:
  std::unique_ptr<net::NetworkDelegate> CreateNetworkDelegate() override;
  std::string GetUserAgent() override;
  std::unique_ptr<net::URLRequestJobFactory> CreateURLRequestJobFactory(
      content::ProtocolHandlerMap* protocol_handlers) override;
  net::HttpCache::BackendFactory* CreateHttpCacheBackendFactory(
      const base::FilePath& base_path) override;
  std::unique_ptr<net::CertVerifier> CreateCertVerifier(
      brightray::RequireCTDelegate* ct_delegate) override;
  std::vector<std::string> GetCookieableSchemes() override;
  void NotifyCookieChange(const net::CanonicalCookie& cookie,
                          bool removed,
                          net::CookieStore::ChangeCause cause) override;

  // content::BrowserContext:
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  content::PermissionManager* GetPermissionManager() override;

  // brightray::BrowserContext:
  void RegisterPrefs(PrefRegistrySimple* pref_registry) override;

  AtomBlobReader* GetBlobReader();

  void set_cookie_change_subscription(
      std::unique_ptr<
          base::CallbackList<void(const CookieDetails*)>::Subscription>
          subscription) {
    cookie_change_subscription_.swap(subscription);
  }

 protected:
  AtomBrowserContext(const std::string& partition, bool in_memory,
                     const base::DictionaryValue& options);
  ~AtomBrowserContext() override;

 private:
  std::unique_ptr<AtomDownloadManagerDelegate> download_manager_delegate_;
  std::unique_ptr<WebViewManager> guest_manager_;
  std::unique_ptr<AtomPermissionManager> permission_manager_;
  std::unique_ptr<AtomBlobReader> blob_reader_;
  std::string user_agent_;
  bool use_cache_;

  base::CallbackList<void(const CookieDetails*)> cookie_change_sub_list_;
  std::unique_ptr<base::CallbackList<void(const CookieDetails*)>::Subscription>
      cookie_change_subscription_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserContext);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
