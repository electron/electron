// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_SESSION_H_
#define SHELL_BROWSER_API_ATOM_API_SESSION_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "content/public/browser/download_manager.h"
#include "electron/buildflags/buildflags.h"
#include "gin/handle.h"
#include "shell/browser/net/resolve_proxy_helper.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/trackable_object.h"

class GURL;

namespace base {
class FilePath;
}

namespace gin_helper {
class Dictionary;
}

namespace net {
class ProxyConfig;
}

namespace electron {

class AtomBrowserContext;

namespace api {

class Session : public gin_helper::TrackableObject<Session>,
                public content::DownloadManager::Observer {
 public:
  // Gets or creates Session from the |browser_context|.
  static gin::Handle<Session> CreateFrom(v8::Isolate* isolate,
                                         AtomBrowserContext* browser_context);

  // Gets the Session of |partition|.
  static gin::Handle<Session> FromPartition(
      v8::Isolate* isolate,
      const std::string& partition,
      base::DictionaryValue options = base::DictionaryValue());

  AtomBrowserContext* browser_context() const { return browser_context_.get(); }

  // gin_helper::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // Methods.
  v8::Local<v8::Promise> ResolveProxy(gin_helper::Arguments* args);
  v8::Local<v8::Promise> GetCacheSize();
  v8::Local<v8::Promise> ClearCache();
  v8::Local<v8::Promise> ClearStorageData(gin_helper::Arguments* args);
  void FlushStorageData();
  v8::Local<v8::Promise> SetProxy(gin_helper::Arguments* args);
  void SetDownloadPath(const base::FilePath& path);
  void EnableNetworkEmulation(const gin_helper::Dictionary& options);
  void DisableNetworkEmulation();
  void SetCertVerifyProc(v8::Local<v8::Value> proc,
                         gin_helper::Arguments* args);
  void SetPermissionRequestHandler(v8::Local<v8::Value> val,
                                   gin_helper::Arguments* args);
  void SetPermissionCheckHandler(v8::Local<v8::Value> val,
                                 gin_helper::Arguments* args);
  v8::Local<v8::Promise> ClearHostResolverCache(gin_helper::Arguments* args);
  v8::Local<v8::Promise> ClearAuthCache();
  void AllowNTLMCredentialsForDomains(const std::string& domains);
  void SetUserAgent(const std::string& user_agent, gin_helper::Arguments* args);
  std::string GetUserAgent();
  v8::Local<v8::Promise> GetBlobData(v8::Isolate* isolate,
                                     const std::string& uuid);
  void DownloadURL(const GURL& url);
  void CreateInterruptedDownload(const gin_helper::Dictionary& options);
  void SetPreloads(const std::vector<base::FilePath::StringType>& preloads);
  std::vector<base::FilePath::StringType> GetPreloads() const;
  v8::Local<v8::Value> Cookies(v8::Isolate* isolate);
  v8::Local<v8::Value> Protocol(v8::Isolate* isolate);
  v8::Local<v8::Value> WebRequest(v8::Isolate* isolate);
  v8::Local<v8::Value> NetLog(v8::Isolate* isolate);
  void Preconnect(const gin_helper::Dictionary& options,
                  gin_helper::Arguments* args);
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  base::Value GetSpellCheckerLanguages();
  void SetSpellCheckerLanguages(gin_helper::ErrorThrower thrower,
                                const std::vector<std::string>& languages);
  bool AddWordToSpellCheckerDictionary(const std::string& word);
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  v8::Local<v8::Promise> LoadExtension(const base::FilePath& extension_path);
  void RemoveExtension(const std::string& extension_id);
  v8::Local<v8::Value> GetExtension(const std::string& extension_id);
  v8::Local<v8::Value> GetAllExtensions();
#endif

 protected:
  Session(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~Session() override;

  // content::DownloadManager::Observer:
  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;

 private:
  // Cached gin_helper::Wrappable objects.
  v8::Global<v8::Value> cookies_;
  v8::Global<v8::Value> protocol_;
  v8::Global<v8::Value> net_log_;

  // Cached object.
  v8::Global<v8::Value> web_request_;

  // The client id to enable the network throttler.
  base::UnguessableToken network_emulation_token_;

  scoped_refptr<AtomBrowserContext> browser_context_;

  DISALLOW_COPY_AND_ASSIGN(Session);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_SESSION_H_
