// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "build/build_config.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/wrappable.h"

namespace v8 {
class Context;
class Isolate;
class Object;
class Value;
template <class T>
class Local;
}  // namespace v8

namespace gin {
class ObjectTemplateBuilder;
}  // namespace gin

namespace gin_helper {
template <typename T>
class Handle;
}  // namespace gin_helper

namespace electron::api {
class SafeStorage final : public gin_helper::DeprecatedWrappable<SafeStorage> {
 public:
  static gin_helper::Handle<SafeStorage> Create(v8::Isolate* isolate);

  // gin_helper::Wrappable
  static gin::DeprecatedWrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // disable copy
  SafeStorage(const SafeStorage&) = delete;
  SafeStorage& operator=(const SafeStorage&) = delete;

 protected:
  explicit SafeStorage(v8::Isolate* isolate);
  ~SafeStorage() override;

 private:
  // Lazily request the async encryptor on first use. ESM named imports
  // eagerly evaluate all electron module getters, so requesting in the
  // constructor would touch the OS keychain even when safeStorage is unused.
  void EnsureAsyncEncryptorRequested();
  bool EnsureSyncOsCryptInitializedForSyncApi();

  void OnOsCryptReady(const os_crypt_async::Encryptor* encryptor);

  bool IsEncryptionAvailableImpl();
  bool IsEncryptionAvailable();

  v8::Local<v8::Promise> IsAsyncEncryptionAvailable(v8::Isolate* isolate);

  void SetUsePasswordV10(bool use);

  v8::Local<v8::Value> EncryptString(v8::Isolate* isolate,
                                     const std::string& plaintext);

  std::string DecryptString(v8::Isolate* isolate, v8::Local<v8::Value> buffer);

  v8::Local<v8::Promise> encryptStringAsync(v8::Isolate* isolate,
                                            const std::string& plaintext);

  v8::Local<v8::Promise> decryptStringAsync(v8::Isolate* isolate,
                                            v8::Local<v8::Value> buffer);
#if BUILDFLAG(IS_LINUX)
  std::string GetSelectedLinuxBackend();
#endif

  bool use_password_v10_ = false;

  bool encryptor_requested_ = false;
  raw_ptr<const os_crypt_async::Encryptor> encryptor_ = nullptr;
#if BUILDFLAG(IS_LINUX)
  bool sync_os_crypt_configured_ = false;
#endif
#if BUILDFLAG(IS_WIN)
  bool sync_os_crypt_initialized_ = false;
#endif

  // Pending encrypt operations waiting for encryptor to be ready.
  struct PendingEncrypt {
    PendingEncrypt(gin_helper::Promise<v8::Local<v8::Value>> promise,
                   std::string plaintext);
    ~PendingEncrypt();
    PendingEncrypt(PendingEncrypt&&);
    PendingEncrypt& operator=(PendingEncrypt&&);

    gin_helper::Promise<v8::Local<v8::Value>> promise;
    std::string plaintext;
  };
  std::vector<PendingEncrypt> pending_encrypts_;

  // Pending decrypt operations waiting for encryptor to be ready.
  struct PendingDecrypt {
    PendingDecrypt(gin_helper::Promise<gin_helper::Dictionary> promise,
                   std::string ciphertext);
    ~PendingDecrypt();
    PendingDecrypt(PendingDecrypt&&);
    PendingDecrypt& operator=(PendingDecrypt&&);

    gin_helper::Promise<gin_helper::Dictionary> promise;
    std::string ciphertext;
  };
  std::vector<PendingDecrypt> pending_decrypts_;

  std::vector<gin_helper::Promise<bool>> pending_availability_checks_;
};

}  // namespace electron::api

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv);

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_
