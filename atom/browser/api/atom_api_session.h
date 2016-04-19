// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_SESSION_H_
#define ATOM_BROWSER_API_ATOM_API_SESSION_H_

#include <string>

#include "atom/browser/api/trackable_object.h"
#include "content/public/browser/download_manager.h"
#include "native_mate/handle.h"
#include "net/base/completion_callback.h"

class GURL;

namespace base {
class FilePath;
}

namespace mate {
class Arguments;
class Dictionary;
}

namespace net {
class ProxyConfig;
}

namespace atom {

class AtomBrowserContext;

namespace api {

class Session: public mate::TrackableObject<Session>,
               public content::DownloadManager::Observer {
 public:
  using ResolveProxyCallback = base::Callback<void(std::string)>;

  enum class CacheAction {
    CLEAR,
    STATS,
  };

  // Gets or creates Session from the |browser_context|.
  static mate::Handle<Session> CreateFrom(
      v8::Isolate* isolate, AtomBrowserContext* browser_context);

  // Gets the Session of |partition| and |in_memory|.
  static mate::Handle<Session> FromPartition(
      v8::Isolate* isolate, const std::string& partition, bool in_memory);

  AtomBrowserContext* browser_context() const { return browser_context_.get(); }

  // mate::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

 protected:
  explicit Session(AtomBrowserContext* browser_context);
  ~Session();

  // content::DownloadManager::Observer:
  void OnDownloadCreated(content::DownloadManager* manager,
                         content::DownloadItem* item) override;

 private:
  void ResolveProxy(const GURL& url, ResolveProxyCallback callback);
  template<CacheAction action>
  void DoCacheAction(const net::CompletionCallback& callback);
  void ClearStorageData(mate::Arguments* args);
  void FlushStorageData();
  void SetProxy(const net::ProxyConfig& config, const base::Closure& callback);
  void SetDownloadPath(const base::FilePath& path);
  void EnableNetworkEmulation(const mate::Dictionary& options);
  void DisableNetworkEmulation();
  void SetCertVerifyProc(v8::Local<v8::Value> proc, mate::Arguments* args);
  void SetPermissionRequestHandler(v8::Local<v8::Value> val,
                                   mate::Arguments* args);
  void ClearHostResolverCache(mate::Arguments* args);
  v8::Local<v8::Value> Cookies(v8::Isolate* isolate);
  v8::Local<v8::Value> WebRequest(v8::Isolate* isolate);

  // Cached object.
  v8::Global<v8::Value> cookies_;
  v8::Global<v8::Value> web_request_;

  // The X-DevTools-Emulate-Network-Conditions-Client-Id.
  std::string devtools_network_emulation_client_id_;

  scoped_refptr<AtomBrowserContext> browser_context_;

  DISALLOW_COPY_AND_ASSIGN(Session);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_SESSION_H_
