// Copyright (c) 2024 Switchboard
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_crypto_module_delegate_nss.h"

#include "base/functional/bind.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/nss_crypto_module_delegate.h"
#include "shell/browser/api/electron_api_app.h"

ElectronNSSCryptoModuleDelegate::ElectronNSSCryptoModuleDelegate(
    const net::HostPortPair& server)
    : server_(server),
      event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
             base::WaitableEvent::InitialState::NOT_SIGNALED) {}

ElectronNSSCryptoModuleDelegate::~ElectronNSSCryptoModuleDelegate() = default;

std::string ElectronNSSCryptoModuleDelegate::RequestPassword(
    const std::string& token_name,
    bool retry,
    bool* cancelled) {
  DCHECK(!event_.IsSignaled());
  event_.Reset();

  if (content::GetUIThreadTaskRunner({})->PostTask(
          FROM_HERE,
          base::BindOnce(
              &ElectronNSSCryptoModuleDelegate::RequestPasswordOnUIThread, this,
              token_name, retry))) {
    base::ScopedAllowBaseSyncPrimitivesForTesting allow_wait;
    event_.Wait();
  }
  *cancelled = cancelled_;
  return password_;
}

void ElectronNSSCryptoModuleDelegate::RequestPasswordOnUIThread(
    const std::string& token_name,
    bool retry) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!electron::api::App::Get()->RequestClientCertPassword(
          server_.host(), token_name, retry,
          base::BindOnce(&ElectronNSSCryptoModuleDelegate::OnPassword, this))) {
    OnPassword(std::string());
  }
}

void ElectronNSSCryptoModuleDelegate::OnPassword(const std::string& password) {
  password_ = password;
  cancelled_ = password_.empty();
  event_.Signal();
}
