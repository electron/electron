// Copyright (c) 2024 Switchboard
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_CRYPTO_MODULE_DELEGATE_NSS_H_
#define ELECTRON_SHELL_CRYPTO_MODULE_DELEGATE_NSS_H_

#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_restrictions.h"
#include "crypto/nss_crypto_module_delegate.h"
#include "net/base/host_port_pair.h"

namespace gin {
class Arguments;
}

class ElectronNSSCryptoModuleDelegate
    : public crypto::CryptoModuleBlockingPasswordDelegate {
 public:
  explicit ElectronNSSCryptoModuleDelegate(const net::HostPortPair& server);
  ElectronNSSCryptoModuleDelegate(const ElectronNSSCryptoModuleDelegate&) =
      delete;
  ElectronNSSCryptoModuleDelegate& operator=(
      const ElectronNSSCryptoModuleDelegate&) = delete;

  std::string RequestPassword(const std::string& token_name,
                              bool retry,
                              bool* cancelled) override;

 private:
  friend class base::RefCountedThreadSafe<ElectronNSSCryptoModuleDelegate>;
  ~ElectronNSSCryptoModuleDelegate() override;

  void RequestPasswordOnUIThread(const std::string& token_name, bool retry);
  void OnPassword(gin::Arguments* args);

  net::HostPortPair server_;
  base::WaitableEvent event_;
  std::string password_;
  bool cancelled_ = false;
};

#endif  // ELECTRON_SHELL_CRYPTO_MODULE_DELEGATE_NSS_H_
