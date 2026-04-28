// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "shell/browser/api/electron_api_safe_storage.h"

#include "base/command_line.h"
#include "base/environment.h"
#include "base/functional/bind.h"
#include "base/nix/xdg_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/sync/key_storage_config_linux.h"
#include "components/os_crypt/sync/key_storage_util_linux.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/password_manager/core/browser/password_manager_switches.h"  // nogncheck
#include "gin/object_template_builder.h"
#include "shell/browser/browser.h"
#include "shell/browser/browser_process_impl.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/os_crypt_async_encryptor_provider.h"
#include "shell/common/application_info.h"
#include "shell/common/electron_paths.h"
#include "shell/common/gin_converters/base_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"

namespace {

const char* kEncryptionVersionPrefixV10 = "v10";
const char* kEncryptionVersionPrefixV11 = "v11";

void EmitIsEncryptionAvailableDeprecation(v8::Isolate* isolate) {
  static bool deprecated_warning_issued = false;
  if (!deprecated_warning_issued) {
    deprecated_warning_issued = true;
    electron::util::EmitDeprecationWarning(
        isolate,
        "safeStorage.isEncryptionAvailable() is deprecated, use "
        "safeStorage.isAsyncEncryptionAvailable() instead.",
        "ELECTRON_DEPRECATED_SAFE_STORAGE_SYNC_API");
  }
}

void EmitEncryptStringDeprecation(v8::Isolate* isolate) {
  static bool deprecated_warning_issued = false;
  if (!deprecated_warning_issued) {
    deprecated_warning_issued = true;
    electron::util::EmitDeprecationWarning(
        isolate,
        "safeStorage.encryptString() is deprecated, use "
        "safeStorage.encryptStringAsync() instead.",
        "ELECTRON_DEPRECATED_SAFE_STORAGE_SYNC_API");
  }
}

void EmitDecryptStringDeprecation(v8::Isolate* isolate) {
  static bool deprecated_warning_issued = false;
  if (!deprecated_warning_issued) {
    deprecated_warning_issued = true;
    electron::util::EmitDeprecationWarning(
        isolate,
        "safeStorage.decryptString() is deprecated, use "
        "safeStorage.decryptStringAsync() instead.",
        "ELECTRON_DEPRECATED_SAFE_STORAGE_SYNC_API");
  }
}

}  // namespace

namespace electron::api {

gin::DeprecatedWrapperInfo SafeStorage::kWrapperInfo = {
    gin::kEmbedderNativeGin};

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

gin_helper::Handle<SafeStorage> SafeStorage::Create(v8::Isolate* isolate) {
  return gin_helper::CreateHandle(isolate, new SafeStorage(isolate));
}

SafeStorage::SafeStorage(v8::Isolate* isolate) {}

SafeStorage::~SafeStorage() = default;

gin::ObjectTemplateBuilder SafeStorage::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::DeprecatedWrappable<SafeStorage>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("isEncryptionAvailable", &SafeStorage::IsEncryptionAvailable)
      .SetMethod("isAsyncEncryptionAvailable",
                 &SafeStorage::IsAsyncEncryptionAvailable)
      .SetMethod("setUsePlainTextEncryption", &SafeStorage::SetUsePasswordV10)
      .SetMethod("encryptString", &SafeStorage::EncryptString)
      .SetMethod("decryptString", &SafeStorage::DecryptString)
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
  static_cast<BrowserProcessImpl*>(g_browser_process)
      ->os_crypt_async_encryptor_provider()
      ->GetEncryptor(
          base::BindOnce(&SafeStorage::OnOsCryptReady, base::Unretained(this)),
          os_crypt_async::Encryptor::Option::kEncryptSyncCompat);
}

bool SafeStorage::EnsureSyncOsCryptInitializedForSyncApi() {
#if BUILDFLAG(IS_LINUX)
  if (!sync_os_crypt_configured_) {
    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();
    std::unique_ptr<os_crypt::Config> config =
        std::make_unique<os_crypt::Config>();
    config->store =
        command_line.GetSwitchValueASCII(password_manager::kPasswordStore);
    config->product_name = electron::GetApplicationName();
    config->application_name = electron::GetApplicationName();
    config->should_use_preference =
        command_line.HasSwitch(password_manager::kEnableEncryptionSelection);
    base::PathService::Get(electron::DIR_SESSION_DATA, &config->user_data_path);

    const bool use_backend = !config->should_use_preference ||
                             os_crypt::GetBackendUse(config->user_data_path);
    std::unique_ptr<base::Environment> env(base::Environment::Create());
    const base::nix::DesktopEnvironment desktop_env =
        base::nix::GetDesktopEnvironment(env.get());
    const os_crypt::SelectedLinuxBackend selected_backend =
        os_crypt::SelectBackend(config->store, use_backend, desktop_env);
    auto* browser_process = static_cast<BrowserProcessImpl*>(g_browser_process);
    switch (selected_backend) {
      case os_crypt::SelectedLinuxBackend::BASIC_TEXT:
        browser_process->SetLinuxStorageBackend("basic_text");
        break;
      case os_crypt::SelectedLinuxBackend::GNOME_LIBSECRET:
        browser_process->SetLinuxStorageBackend("gnome_libsecret");
        break;
      case os_crypt::SelectedLinuxBackend::KWALLET:
        browser_process->SetLinuxStorageBackend("kwallet");
        break;
      case os_crypt::SelectedLinuxBackend::KWALLET5:
        browser_process->SetLinuxStorageBackend("kwallet5");
        break;
      case os_crypt::SelectedLinuxBackend::KWALLET6:
        browser_process->SetLinuxStorageBackend("kwallet6");
        break;
      case os_crypt::SelectedLinuxBackend::DEFER:
        browser_process->SetLinuxStorageBackend("unknown");
        break;
    }

    OSCrypt::SetConfig(std::move(config));
    sync_os_crypt_configured_ = true;
  }
#endif

#if BUILDFLAG(IS_WIN)
  if (sync_os_crypt_initialized_)
    return true;

  auto* local_state = g_browser_process->local_state();
  if (!local_state)
    return false;

  sync_os_crypt_initialized_ = OSCrypt::Init(local_state);
  return sync_os_crypt_initialized_;
#else
  return true;
#endif
}

void SafeStorage::OnOsCryptReady(const os_crypt_async::Encryptor* encryptor) {
  encryptor_ = encryptor;

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

const char* SafeStorage::GetTypeName() {
  return "SafeStorage";
}

bool SafeStorage::IsEncryptionAvailableImpl() {
  if (!electron::Browser::Get()->is_ready())
    return false;
  if (!EnsureSyncOsCryptInitializedForSyncApi())
    return false;
#if BUILDFLAG(IS_LINUX)
  return OSCrypt::IsEncryptionAvailable() ||
         (use_password_v10_ &&
          static_cast<BrowserProcessImpl*>(g_browser_process)
                  ->linux_storage_backend() == "basic_text");
#else
  return OSCrypt::IsEncryptionAvailable();
#endif
}

bool SafeStorage::IsEncryptionAvailable() {
  EmitIsEncryptionAvailableDeprecation(JavascriptEnvironment::GetIsolate());
  return IsEncryptionAvailableImpl();
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
  EnsureSyncOsCryptInitializedForSyncApi();
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
  EnsureSyncOsCryptInitializedForSyncApi();
  return static_cast<BrowserProcessImpl*>(g_browser_process)
      ->linux_storage_backend();
}
#endif

v8::Local<v8::Value> SafeStorage::EncryptString(v8::Isolate* isolate,
                                                const std::string& plaintext) {
  EmitEncryptStringDeprecation(isolate);
  if (!IsEncryptionAvailableImpl()) {
    if (!electron::Browser::Get()->is_ready()) {
      gin_helper::ErrorThrower(isolate).ThrowError(
          "safeStorage cannot be used before app is ready");
      return {};
    }
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while encrypting the text provided to "
        "safeStorage.encryptString. "
        "Encryption is not available.");
    return {};
  }

  std::string ciphertext;
  bool encrypted = OSCrypt::EncryptString(plaintext, &ciphertext);

  if (!encrypted) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while encrypting the text provided to "
        "safeStorage.encryptString.");
    return {};
  }

  return electron::Buffer::Copy(isolate, ciphertext).ToLocalChecked();
}

std::string SafeStorage::DecryptString(v8::Isolate* isolate,
                                       v8::Local<v8::Value> buffer) {
  EmitDecryptStringDeprecation(isolate);
  if (!IsEncryptionAvailableImpl()) {
    if (!electron::Browser::Get()->is_ready()) {
      gin_helper::ErrorThrower(isolate).ThrowError(
          "safeStorage cannot be used before app is ready");
      return "";
    }
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while decrypting the ciphertext provided to "
        "safeStorage.decryptString. "
        "Decryption is not available.");
    return "";
  }

  if (!node::Buffer::HasInstance(buffer)) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Expected the first argument of decryptString() to be a buffer");
    return "";
  }

  // ensures an error is thrown in Mac or Linux on
  // decryption failure, rather than failing silently
  const char* data = node::Buffer::Data(buffer);
  auto size = node::Buffer::Length(buffer);
  std::string ciphertext(data, size);
  if (ciphertext.empty()) {
    return "";
  }

  if (ciphertext.find(kEncryptionVersionPrefixV10) != 0 &&
      ciphertext.find(kEncryptionVersionPrefixV11) != 0) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while decrypting the ciphertext provided to "
        "safeStorage.decryptString. "
        "Ciphertext does not appear to be encrypted.");
    return "";
  }

  std::string plaintext;
  bool decrypted = OSCrypt::DecryptString(ciphertext, &plaintext);
  if (!decrypted) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while decrypting the ciphertext provided to "
        "safeStorage.decryptString.");
    return "";
  }
  return plaintext;
}

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
