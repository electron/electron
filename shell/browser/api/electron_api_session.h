// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SESSION_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SESSION_H_

#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "content/public/browser/download_manager.h"
#include "electron/buildflags/buildflags.h"
#include "gin/wrappable.h"
#include "services/network/public/mojom/host_resolver.mojom-forward.h"
#include "services/network/public/mojom/ssl_config.mojom-forward.h"
#include "shell/browser/api/ipc_dispatcher.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/net/resolve_proxy_helper.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/pinnable.h"

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "chrome/browser/spellchecker/spellcheck_hunspell_dictionary.h"  // nogncheck
#endif

class GURL;

namespace base {
class FilePath;
}

namespace gin {
class Arguments;
template <typename T>
class Handle;
}  // namespace gin

namespace gin_helper {
class Dictionary;
class ErrorThrower;
}  // namespace gin_helper

namespace net {
class ProxyConfig;
}

namespace electron {

class ElectronBrowserContext;
struct PreloadScript;

namespace api {

class Session final : public gin::Wrappable<Session>,
                      public gin_helper::Pinnable<Session>,
                      public gin_helper::Constructible<Session>,
                      public gin_helper::EventEmitterMixin<Session>,
                      public gin_helper::CleanedUpAtExit,
                      public IpcDispatcher<Session>,
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
                      private SpellcheckHunspellDictionary::Observer,
#endif
                      private content::DownloadManager::Observer {
 public:
  // Gets or creates Session from the |browser_context|.
  static gin::Handle<Session> CreateFrom(
      v8::Isolate* isolate,
      ElectronBrowserContext* browser_context);
  static gin::Handle<Session> New();  // Dummy, do not use!

  static Session* FromBrowserContext(content::BrowserContext* context);

  // Gets the Session of |partition|.
  static gin::Handle<Session> FromPartition(v8::Isolate* isolate,
                                            const std::string& partition,
                                            base::Value::Dict options = {});

  // Gets the Session based on |path|.
  static std::optional<gin::Handle<Session>> FromPath(
      v8::Isolate* isolate,
      const base::FilePath& path,
      base::Value::Dict options = {});

  ElectronBrowserContext* browser_context() const {
    return &browser_context_.get();
  }

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  static void FillObjectTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static const char* GetClassName() { return "Session"; }
  const char* GetTypeName() override;

  // gin_helper::CleanedUpAtExit
  void WillBeDestroyed() override;

  // Methods.
  v8::Local<v8::Promise> ResolveHost(
      std::string host,
      std::optional<network::mojom::ResolveHostParametersPtr> params);
  v8::Local<v8::Promise> ResolveProxy(gin::Arguments* args);
  v8::Local<v8::Promise> GetCacheSize();
  v8::Local<v8::Promise> ClearCache();
  v8::Local<v8::Promise> ClearStorageData(gin::Arguments* args);
  void FlushStorageData();
  v8::Local<v8::Promise> SetProxy(gin::Arguments* args);
  v8::Local<v8::Promise> ForceReloadProxyConfig();
  void SetDownloadPath(const base::FilePath& path);
  void EnableNetworkEmulation(const gin_helper::Dictionary& options);
  void DisableNetworkEmulation();
  void SetCertVerifyProc(v8::Local<v8::Value> proc, gin::Arguments* args);
  void SetPermissionRequestHandler(v8::Local<v8::Value> val,
                                   gin::Arguments* args);
  void SetPermissionCheckHandler(v8::Local<v8::Value> val,
                                 gin::Arguments* args);
  void SetDevicePermissionHandler(v8::Local<v8::Value> val,
                                  gin::Arguments* args);
  void SetUSBProtectedClassesHandler(v8::Local<v8::Value> val,
                                     gin::Arguments* args);
  void SetBluetoothPairingHandler(v8::Local<v8::Value> val,
                                  gin::Arguments* args);
  v8::Local<v8::Promise> ClearHostResolverCache(gin::Arguments* args);
  v8::Local<v8::Promise> ClearAuthCache();
  void AllowNTLMCredentialsForDomains(const std::string& domains);
  void SetUserAgent(const std::string& user_agent, gin::Arguments* args);
  std::string GetUserAgent();
  void SetSSLConfig(network::mojom::SSLConfigPtr config);
  bool IsPersistent();
  v8::Local<v8::Promise> GetBlobData(v8::Isolate* isolate,
                                     const std::string& uuid);
  void DownloadURL(const GURL& url, gin::Arguments* args);
  void CreateInterruptedDownload(const gin_helper::Dictionary& options);
  std::string RegisterPreloadScript(gin_helper::ErrorThrower thrower,
                                    const PreloadScript& new_preload_script);
  void UnregisterPreloadScript(gin_helper::ErrorThrower thrower,
                               const std::string& script_id);
  std::vector<PreloadScript> GetPreloadScripts() const;
  v8::Local<v8::Promise> GetSharedDictionaryInfo(
      const gin_helper::Dictionary& options);
  v8::Local<v8::Promise> GetSharedDictionaryUsageInfo();
  v8::Local<v8::Promise> ClearSharedDictionaryCache();
  v8::Local<v8::Promise> ClearSharedDictionaryCacheForIsolationKey(
      const gin_helper::Dictionary& options);
  v8::Local<v8::Value> Cookies(v8::Isolate* isolate);
  v8::Local<v8::Value> Extensions(v8::Isolate* isolate);
  v8::Local<v8::Value> Protocol(v8::Isolate* isolate);
  v8::Local<v8::Value> ServiceWorkerContext(v8::Isolate* isolate);
  v8::Local<v8::Value> WebRequest(v8::Isolate* isolate);
  v8::Local<v8::Value> NetLog(v8::Isolate* isolate);
  void Preconnect(const gin_helper::Dictionary& options, gin::Arguments* args);
  v8::Local<v8::Promise> CloseAllConnections();
  v8::Local<v8::Value> GetPath(v8::Isolate* isolate);
  void SetCodeCachePath(gin::Arguments* args);
  v8::Local<v8::Promise> ClearCodeCaches(const gin_helper::Dictionary& options);
  v8::Local<v8::Value> ClearData(gin_helper::ErrorThrower thrower,
                                 gin::Arguments* args);
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  base::Value GetSpellCheckerLanguages();
  void SetSpellCheckerLanguages(gin_helper::ErrorThrower thrower,
                                const std::vector<std::string>& languages);
  v8::Local<v8::Promise> ListWordsInSpellCheckerDictionary();
  bool AddWordToSpellCheckerDictionary(const std::string& word);
  bool RemoveWordFromSpellCheckerDictionary(const std::string& word);
  void SetSpellCheckerEnabled(bool b);
  bool IsSpellCheckerEnabled() const;
#endif

  // disable copy
  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;

 protected:
  Session(v8::Isolate* isolate, ElectronBrowserContext* browser_context);
  ~Session() override;

  // content::DownloadManager::Observer:
  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  // SpellcheckHunspellDictionary::Observer
  void OnHunspellDictionaryInitialized(const std::string& language) override;
  void OnHunspellDictionaryDownloadBegin(const std::string& language) override;
  void OnHunspellDictionaryDownloadSuccess(
      const std::string& language) override;
  void OnHunspellDictionaryDownloadFailure(
      const std::string& language) override;
#endif

 private:
  void SetDisplayMediaRequestHandler(v8::Isolate* isolate,
                                     v8::Local<v8::Value> val);

  // Cached gin_helper::Wrappable objects.
  v8::Global<v8::Value> cookies_;
  v8::Global<v8::Value> extensions_;
  v8::Global<v8::Value> protocol_;
  v8::Global<v8::Value> net_log_;
  v8::Global<v8::Value> service_worker_context_;
  v8::Global<v8::Value> web_request_;

  raw_ptr<v8::Isolate> isolate_;

  // The client id to enable the network throttler.
  base::UnguessableToken network_emulation_token_;

  const raw_ref<ElectronBrowserContext> browser_context_;

  base::WeakPtrFactory<Session> weak_factory_{this};
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SESSION_H_
