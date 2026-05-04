// Copyright (c) 2026 Electron contributors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_OS_CRYPT_ASYNC_ENCRYPTOR_PROVIDER_H_
#define ELECTRON_SHELL_BROWSER_OS_CRYPT_ASYNC_ENCRYPTOR_PROVIDER_H_

#include <optional>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/os_crypt/async/common/encryptor.h"

namespace os_crypt_async {
class OSCryptAsync;
}

namespace electron {

// Provides a shared, cached async encryptor instance for browser-process code.
class OSCryptAsyncEncryptorProvider {
 public:
  using EncryptorCallback =
      base::OnceCallback<void(const os_crypt_async::Encryptor*)>;

  explicit OSCryptAsyncEncryptorProvider(
      os_crypt_async::OSCryptAsync* os_crypt_async);
  ~OSCryptAsyncEncryptorProvider();

  OSCryptAsyncEncryptorProvider(const OSCryptAsyncEncryptorProvider&) = delete;
  OSCryptAsyncEncryptorProvider& operator=(
      const OSCryptAsyncEncryptorProvider&) = delete;

  void GetEncryptor(EncryptorCallback callback,
                    os_crypt_async::Encryptor::Option option =
                        os_crypt_async::Encryptor::Option::kEncryptSyncCompat);

 private:
  void EnsureEncryptorRequested(os_crypt_async::Encryptor::Option option);
  void OnEncryptorReady(os_crypt_async::Encryptor encryptor);

  raw_ptr<os_crypt_async::OSCryptAsync> os_crypt_async_;
  bool encryptor_requested_ = false;
  os_crypt_async::Encryptor::Option requested_option_ =
      os_crypt_async::Encryptor::Option::kEncryptSyncCompat;
  std::optional<os_crypt_async::Encryptor> encryptor_;
  std::vector<EncryptorCallback> pending_callbacks_;
  base::WeakPtrFactory<OSCryptAsyncEncryptorProvider> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_OS_CRYPT_ASYNC_ENCRYPTOR_PROVIDER_H_
