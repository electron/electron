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
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "services/network/public/mojom/host_resolver.mojom-forward.h"
#include "services/network/public/mojom/ssl_config.mojom-forward.h"
#include "shell/browser/api/ipc_dispatcher.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/net/resolve_proxy_helper.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/self_keep_alive.h"

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "chrome/browser/spellchecker/spellcheck_hunspell_dictionary.h"  // nogncheck
#endif

class GURL;

namespace base {
class FilePath;
}

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
class Dictionary;
class ErrorThrower;
}  // namespace gin_helper

namespace net {
class ProxyConfig;
}

namespace v8 {
template <typename T>
class TracedReference;
}

namespace electron {

class ElectronBrowserContext;
struct PreloadScript;

namespace api {

class NetLog;
class WebRequest;

class Session final : public gin::Wrappable<Session>,
                      public gin_helper::Constructible<Session>,
                      public gin_helper::EventEmitterMixin<Session>,
                      public gin::PerIsolateData::DisposeObserver,
                      public IpcDispatcher<Session>,
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
                      private SpellcheckHunspellDictionary::Observer,
#endif
                      private content::DownloadManager::Observer {
 public:
  // Gets or creates Session from the |browser_context|.
  static Session* FromOrCreate(v8::Isolate* isolate,
                               ElectronBrowserContext* browser_context);

  // Convenience wrapper around the previous method: Checks that
  // |browser_context| is an ElectronBrowserContext before downcasting.
  static Session* FromOrCreate(v8::Isolate* isolate,
                               content::BrowserContext* browser_context);

  static void New();  // Dummy, do not use!

  static gin::WeakCell<Session>* FromBrowserContext(
      content::BrowserContext* context);

  // Gets the Session of |partition|.
  static Session* FromPartition(v8::Isolate* isolate,
                                const std::string& partition,
                                base::DictValue options = {});

  // Gets the Session based on |path|.
  static Session* FromPath(gin::Arguments* args,
                           const base::FilePath& path,
                           base::DictValue options = {});

  static void FillObjectTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static const char* GetClassName() { return "Session"; }

  Session(v8::Isolate* isolate, ElectronBrowserContext* browser_context);
  ~Session() override;

  ElectronBrowserContext* browser_context() const {
    return &browser_context_.get();
  }

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  void Trace(cppgc::Visitor*) const override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;

  // gin::PerIsolateData::DisposeObserver
  void OnBeforeDispose(v8::Isolate* isolate) override {}
  void OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) override;
  void OnDisposed() override {}

  // Methods.
  void Dispose();
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
  WebRequest* WebRequest(v8::Isolate* isolate);
  api::NetLog* NetLog(v8::Isolate* isolate);
  void Preconnect(const gin_helper::Dictionary& options, gin::Arguments* args);
  v8::Local<v8::Promise> CloseAllConnections();
  v8::Local<v8::Value> GetPath(v8::Isolate* isolate);
  void SetCodeCachePath(gin::Arguments* args);
  v8::Local<v8::Promise> ClearCodeCaches(const gin_helper::Dictionary& options);
  v8::Local<v8::Value> ClearData(gin::Arguments* args);
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
  v8::TracedReference<v8::Value> cookies_;
  v8::TracedReference<v8::Value> extensions_;
  v8::TracedReference<v8::Value> protocol_;
  cppgc::Member<api::NetLog> net_log_;
  v8::TracedReference<v8::Value> service_worker_context_;
  cppgc::Member<api::WebRequest> web_request_;

  raw_ptr<v8::Isolate> isolate_;

  // The client id to enable the network throttler.
  base::UnguessableToken network_emulation_token_;

  const raw_ref<ElectronBrowserContext> browser_context_;

  gin::WeakCellFactory<Session> weak_factory_{this};

  gin_helper::SelfKeepAlive<Session> keep_alive_{this};
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SESSION_H_
