// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "shell/browser/api/electron_api_safe_storage.h"

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "gin/object_template_builder.h"
#include "gin/persistent.h"
#include "shell/browser/browser.h"
#include "shell/browser/browser_process_impl.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/base_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-cppgc.h"

namespace electron::api {

gin::WrapperInfo SafeStorage::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronSafeStorage);

SafeStorage::PendingEncrypt::PendingEncrypt(
    gin_helper::Promise<v8::Local<v8::Value>> promise,
    std::string plaintext)
    : promise(std::move(promise)), plaintext(std::move(plaintext)) {}
SafeStorage::PendingEncrypt::~PendingEncrypt() = default;
SafeStorage::PendingEncrypt::PendingEncrypt(PendingEncrypt&&) = default;
SafeStorage::PendingEncrypt& SafeStorage::PendingEncrypt::operator=(
    PendingEncrypt&&) = default;

SafeStorage::PendingDecrypt::PendingDecrypt(
    gin_helper::Promise<gin_helper::Dictionary> promise,
    std::string ciphertext)
    : promise(std::move(promise)), ciphertext(std::move(ciphertext)) {}
SafeStorage::PendingDecrypt::~PendingDecrypt() = default;
SafeStorage::PendingDecrypt::PendingDecrypt(PendingDecrypt&&) = default;
SafeStorage::PendingDecrypt& SafeStorage::PendingDecrypt::operator=(
    PendingDecrypt&&) = default;

SafeStorage* SafeStorage::Create(v8::Isolate* isolate) {
  static base::NoDestructor<cppgc::Persistent<SafeStorage>> instance([isolate] {
    return cppgc::Persistent<SafeStorage>(
        cppgc::MakeGarbageCollected<SafeStorage>(
            isolate->GetCppHeap()->GetAllocationHandle(), isolate));
  }());
  return instance->Get();
}

SafeStorage::SafeStorage(v8::Isolate* isolate) {
  gin::PerIsolateData::From(isolate)->AddDisposeObserver(this);
}

SafeStorage::~SafeStorage() = default;

void SafeStorage::OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) {
  gin::PerIsolateData::From(isolate)->RemoveDisposeObserver(this);
  weak_factory_.Invalidate();
  pending_availability_checks_.clear();
  pending_encrypts_.clear();
  pending_decrypts_.clear();
}

gin::ObjectTemplateBuilder SafeStorage::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::ObjectTemplateBuilder(isolate, GetClassName())
      .SetMethod("isAsyncEncryptionAvailable",
                 &SafeStorage::IsAsyncEncryptionAvailable)
      .SetMethod("setUsePlainTextEncryption", &SafeStorage::SetUsePasswordV10)
      .SetMethod("encryptStringAsync", &SafeStorage::encryptStringAsync)
      .SetMethod("decryptStringAsync", &SafeStorage::decryptStringAsync)
#if BUILDFLAG(IS_LINUX)
      .SetMethod("getSelectedStorageBackend",
                 &SafeStorage::GetSelectedLinuxBackend)
#endif
      ;
}

void SafeStorage::EnsureAsyncEncryptorRequested() {
  DCHECK(electron::Browser::Get()->is_ready());
  if (encryptor_requested_)
    return;
  encryptor_requested_ = true;
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  g_browser_process->os_crypt_async()->GetInstance(
      base::BindOnce(&SafeStorage::OnOsCryptReady,
                     gin::WrapPersistent(weak_factory_.GetWeakCell(
                         isolate->GetCppHeap()->GetAllocationHandle()))));
}

void SafeStorage::OnOsCryptReady(
    scoped_refptr<os_crypt_async::Encryptor> encryptor) {
  encryptor_ = std::move(encryptor);

  // This callback may fire from a posted task without an active V8 scope.
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);

  for (auto& pending : pending_availability_checks_) {
    pending.Resolve(true);
  }
  pending_availability_checks_.clear();

  for (auto& pending : pending_encrypts_) {
    std::string ciphertext;
    bool encrypted = encryptor_->EncryptString(pending.plaintext, &ciphertext);
    if (encrypted) {
      pending.promise.Resolve(
          electron::Buffer::Copy(isolate, ciphertext).ToLocalChecked());
    } else {
      pending.promise.RejectWithErrorMessage(
          "Error while encrypting the text provided to "
          "safeStorage.encryptStringAsync.");
    }
  }
  pending_encrypts_.clear();

  for (auto& pending : pending_decrypts_) {
    std::string plaintext;
    os_crypt_async::Encryptor::DecryptFlags flags;
    bool decrypted =
        encryptor_->DecryptString(pending.ciphertext, &plaintext, &flags);

    if (decrypted) {
      auto dict = gin_helper::Dictionary::CreateEmpty(isolate);

      dict.Set("shouldReEncrypt", flags.should_reencrypt);
      dict.Set("result", plaintext);

      pending.promise.Resolve(dict);
    } else if (flags.temporarily_unavailable) {
      pending.promise.RejectWithErrorMessage(
          "safeStorage.decryptStringAsync is temporarily unavailable. "
          "Please try again.");
    } else {
      pending.promise.RejectWithErrorMessage(
          "Error while decrypting the ciphertext provided to "
          "safeStorage.decryptStringAsync.");
    }
  }
  pending_decrypts_.clear();
}

const gin::WrapperInfo* SafeStorage::wrapper_info() const {
  return &kWrapperInfo;
}

const char* SafeStorage::GetHumanReadableName() const {
  return "Electron / SafeStorage";
}

void SafeStorage::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<SafeStorage>::Trace(visitor);
  visitor->Trace(weak_factory_);
}

v8::Local<v8::Promise> SafeStorage::IsAsyncEncryptionAvailable(
    v8::Isolate* isolate) {
  gin_helper::Promise<bool> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!electron::Browser::Get()->is_ready()) {
    promise.Resolve(false);
    return handle;
  }

#if BUILDFLAG(IS_LINUX)
  if (use_password_v10_ && static_cast<BrowserProcessImpl*>(g_browser_process)
                                   ->linux_storage_backend() == "basic_text") {
    promise.Resolve(true);
    return handle;
  }
#endif

  EnsureAsyncEncryptorRequested();

  if (encryptor_) {
    promise.Resolve(true);
    return handle;
  }

  pending_availability_checks_.push_back(std::move(promise));
  return handle;
}

void SafeStorage::SetUsePasswordV10(bool use) {
  use_password_v10_ = use;
}

#if BUILDFLAG(IS_LINUX)
std::string SafeStorage::GetSelectedLinuxBackend() {
  if (!electron::Browser::Get()->is_ready())
    return "unknown";
  return static_cast<BrowserProcessImpl*>(g_browser_process)
      ->linux_storage_backend();
}
#endif

v8::Local<v8::Promise> SafeStorage::encryptStringAsync(
    v8::Isolate* isolate,
    const std::string& plaintext) {
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!electron::Browser::Get()->is_ready()) {
    promise.RejectWithErrorMessage(
        "safeStorage cannot be used before app is ready");
    return handle;
  }

  EnsureAsyncEncryptorRequested();

  if (encryptor_) {
    std::string ciphertext;
    bool encrypted = encryptor_->EncryptString(plaintext, &ciphertext);
    if (encrypted) {
      promise.Resolve(
          electron::Buffer::Copy(isolate, ciphertext).ToLocalChecked());
    } else {
      promise.RejectWithErrorMessage(
          "Error while encrypting the text provided to "
          "safeStorage.encryptStringAsync.");
    }
    return handle;
  }

  pending_encrypts_.emplace_back(std::move(promise), std::move(plaintext));
  return handle;
}

v8::Local<v8::Promise> SafeStorage::decryptStringAsync(
    v8::Isolate* isolate,
    v8::Local<v8::Value> buffer) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!electron::Browser::Get()->is_ready()) {
    promise.RejectWithErrorMessage(
        "safeStorage cannot be used before app is ready");
    return handle;
  }

  if (!node::Buffer::HasInstance(buffer)) {
    promise.RejectWithErrorMessage(
        "Expected the first argument of decryptStringAsync() to be a buffer");
    return handle;
  }

  const char* data = node::Buffer::Data(buffer);
  auto size = node::Buffer::Length(buffer);
  std::string ciphertext(data, size);

  if (ciphertext.empty()) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("shouldReEncrypt", false);
    dict.Set("result", "");
    promise.Resolve(dict);
    return handle;
  }

  EnsureAsyncEncryptorRequested();

  if (encryptor_) {
    std::string plaintext;
    os_crypt_async::Encryptor::DecryptFlags flags;
    bool decrypted = encryptor_->DecryptString(ciphertext, &plaintext, &flags);
    if (decrypted) {
      auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
      dict.Set("shouldReEncrypt", flags.should_reencrypt);
      dict.Set("result", plaintext);
      promise.Resolve(dict);
    } else if (flags.temporarily_unavailable) {
      promise.RejectWithErrorMessage(
          "safeStorage.decryptStringAsync is temporarily unavailable. "
          "Please try again.");
    } else {
      promise.RejectWithErrorMessage(
          "Error while decrypting the ciphertext provided to "
          "safeStorage.decryptStringAsync.");
    }
    return handle;
  }

  pending_decrypts_.emplace_back(std::move(promise), std::move(ciphertext));
  return handle;
}

}  // namespace electron::api

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("safeStorage", electron::api::SafeStorage::Create(isolate));
}

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_safe_storage, Initialize)
