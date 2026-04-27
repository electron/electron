// Copyright (c) 2026 Electron contributors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/os_crypt_async_encryptor_provider.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"

namespace electron {

OSCryptAsyncEncryptorProvider::OSCryptAsyncEncryptorProvider(
    os_crypt_async::OSCryptAsync* os_crypt_async)
    : os_crypt_async_(os_crypt_async) {
  CHECK(os_crypt_async_);
}

OSCryptAsyncEncryptorProvider::~OSCryptAsyncEncryptorProvider() = default;

void OSCryptAsyncEncryptorProvider::GetEncryptor(
    EncryptorCallback callback,
    os_crypt_async::Encryptor::Option option) {
  if (encryptor_) {
    CHECK_EQ(requested_option_, option);
    std::move(callback).Run(&encryptor_.value());
    return;
  }

  pending_callbacks_.push_back(std::move(callback));
  EnsureEncryptorRequested(option);
}

void OSCryptAsyncEncryptorProvider::EnsureEncryptorRequested(
    os_crypt_async::Encryptor::Option option) {
  if (encryptor_requested_) {
    CHECK_EQ(requested_option_, option);
    return;
  }

  encryptor_requested_ = true;
  requested_option_ = option;
  os_crypt_async_->GetInstance(
      base::BindOnce(&OSCryptAsyncEncryptorProvider::OnEncryptorReady,
                     weak_factory_.GetWeakPtr()),
      option);
}

void OSCryptAsyncEncryptorProvider::OnEncryptorReady(
    os_crypt_async::Encryptor encryptor) {
  encryptor_ = std::move(encryptor);
  for (auto& callback : pending_callbacks_) {
    std::move(callback).Run(&encryptor_.value());
  }
  pending_callbacks_.clear();
}

}  // namespace electron
