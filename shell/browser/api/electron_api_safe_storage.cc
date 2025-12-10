// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "gin/object_template_builder.h"
#include "shell/browser/api/electron_api_safe_storage.h"
#include "shell/browser/browser.h"
#include "shell/browser/browser_process_impl.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/base_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"

namespace {

const char* kEncryptionVersionPrefixV10 = "v10";
const char* kEncryptionVersionPrefixV11 = "v11";

}  // namespace

namespace electron::api {

gin::DeprecatedWrapperInfo SafeStorage::kWrapperInfo = {
    gin::kEmbedderNativeGin};

gin_helper::Handle<SafeStorage> SafeStorage::Create(v8::Isolate* isolate) {
  return gin_helper::CreateHandle(isolate, new SafeStorage(isolate));
}

SafeStorage::SafeStorage(v8::Isolate* isolate) {
  if (electron::Browser::Get()->is_ready()) {
    OnFinishLaunching({});
  } else {
    Browser::Get()->AddObserver(this);
  }
}

SafeStorage::~SafeStorage() {
  Browser::Get()->RemoveObserver(this);
}

gin::ObjectTemplateBuilder SafeStorage::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::DeprecatedWrappable<SafeStorage>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("isEncryptionAvailable", &SafeStorage::IsEncryptionAvailable)
      .SetMethod("setUsePlainTextEncryption", &SafeStorage::SetUsePasswordV10)
      .SetMethod("encryptString", &SafeStorage::EncryptString)
      .SetMethod("decryptString", &SafeStorage::DecryptString)
#if BUILDFLAG(IS_LINUX)
      .SetMethod("getSelectedStorageBackend",
                 &SafeStorage::GetSelectedLinuxBackend)
#endif
      ;
}

void SafeStorage::OnFinishLaunching(base::Value::Dict launch_info) {
  g_browser_process->os_crypt_async()->GetInstance(
      base::BindOnce(&SafeStorage::OnOsCryptReady, base::Unretained(this)),
      os_crypt_async::Encryptor::Option::kEncryptSyncCompat);
}

void SafeStorage::OnOsCryptReady(os_crypt_async::Encryptor encryptor) {
  encryptor_ = std::move(encryptor);
  is_available_ = true;
  Emit("ready-to-use");
}

const char* SafeStorage::GetTypeName() {
  return "SafeStorage";
}

bool SafeStorage::IsEncryptionAvailable() {
  if (!electron::Browser::Get()->is_ready())
    return false;
#if BUILDFLAG(IS_LINUX)
  return is_available_ || (use_password_v10_ &&
                           static_cast<BrowserProcessImpl*>(g_browser_process)
                                   ->linux_storage_backend() == "basic_text");
#else
  return is_available_;
#endif
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

v8::Local<v8::Value> SafeStorage::EncryptString(v8::Isolate* isolate,
                                                const std::string& plaintext) {
  if (!IsEncryptionAvailable()) {
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
  bool encrypted = encryptor_->EncryptString(plaintext, &ciphertext);

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
  if (!IsEncryptionAvailable()) {
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
  bool decrypted = encryptor_->DecryptString(ciphertext, &plaintext);
  if (!decrypted) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while decrypting the ciphertext provided to "
        "safeStorage.decryptString.");
    return "";
  }
  return plaintext;
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
