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

  // Gets or creates Session from the |browser_context|.
  static mate::Handle<Session> CreateFrom(
      v8::Isolate* isolate, AtomBrowserContext* browser_context);

  // Gets the Session of |partition| and |in_memory|.
  static mate::Handle<Session> FromPartition(
      v8::Isolate* isolate, const std::string& partition, bool in_memory);

  AtomBrowserContext* browser_context() const { return browser_context_.get(); }

 protected:
  explicit Session(AtomBrowserContext* browser_context);
  ~Session();

  // content::DownloadManager::Observer:
  void OnDownloadCreated(content::DownloadManager* manager,
                         content::DownloadItem* item) override;

  // mate::Wrappable:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  bool IsDestroyed() const override;

 private:
  // mate::TrackableObject:
  void Destroy() override;

  void ResolveProxy(const GURL& url, ResolveProxyCallback callback);
  void ClearCache(const net::CompletionCallback& callback);
  void ClearStorageData(mate::Arguments* args);
  void SetProxy(const net::ProxyConfig& config, const base::Closure& callback);
  void SetDownloadPath(const base::FilePath& path);
  void EnableNetworkEmulation(const mate::Dictionary& options);
  void DisableNetworkEmulation();
  v8::Local<v8::Value> Cookies(v8::Isolate* isolate);

  // Cached object for cookies API.
  v8::Global<v8::Value> cookies_;

  scoped_refptr<AtomBrowserContext> browser_context_;

  DISALLOW_COPY_AND_ASSIGN(Session);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_SESSION_H_
