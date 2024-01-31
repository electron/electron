// Copyright (c) 2024 Switchboard
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_CRYPTO_MODULE_DELEGATE_NSS_H_
#define ELECTRON_SHELL_CRYPTO_MODULE_DELEGATE_NSS_H_

#include "base/synchronization/waitable_event.h"
#include "crypto/nss_crypto_module_delegate.h"
#include "net/base/host_port_pair.h"

namespace gin {
class Arguments;
}

// RequestPassword need acces access to base sync primitives.
// ScopedAllowBaseSyncPrimitives is a friend of ChromeNSSCryptoModuleDelegate,
// the chrome implementation of CryptoModuleBlockingPasswordDelegate. By naming
// our class CryptoModuleBlockingPasswordDelegate in the global namespace, we
// avoid the need to modify base::ScopedAllowBaseSyncPrimitives
class ChromeNSSCryptoModuleDelegate
    : public crypto::CryptoModuleBlockingPasswordDelegate {
 public:
  explicit ChromeNSSCryptoModuleDelegate(const net::HostPortPair& server);
  ChromeNSSCryptoModuleDelegate(const ChromeNSSCryptoModuleDelegate&) = delete;
  ChromeNSSCryptoModuleDelegate& operator=(
      const ChromeNSSCryptoModuleDelegate&) = delete;

  std::string RequestPassword(const std::string& token_name,
                              bool retry,
                              bool* cancelled) override;

 private:
  friend class base::RefCountedThreadSafe<ChromeNSSCryptoModuleDelegate>;
  ~ChromeNSSCryptoModuleDelegate() override;

  void RequestPasswordOnUIThread(const std::string& token_name, bool retry);
  void OnPassword(gin::Arguments* args);

  net::HostPortPair server_;
  base::WaitableEvent event_;
  std::string password_;
  bool cancelled_ = false;
};

#endif  // ELECTRON_SHELL_CRYPTO_MODULE_DELEGATE_NSS_H_
