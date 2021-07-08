// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_safe_storage.h"

#include "components/os_crypt/os_crypt.h"
#include "shell/browser/browser.h"
#include "shell/common/gin_converters/base_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "shell/common/platform_util.h"

namespace electron {

namespace api {

#if DCHECK_IS_ON()
bool SafeStorage::electron_crypto_ready = false;
#endif

gin::WrapperInfo SafeStorage::kWrapperInfo = {gin::kEmbedderNativeGin};

electron::api::SafeStorage::SafeStorage() {}
electron::api::SafeStorage::~SafeStorage() = default;

bool SafeStorage::IsEncryptionAvailable() {
  return OSCrypt::IsEncryptionAvailable();
}

v8::Local<v8::Value> SafeStorage::EncryptString(v8::Isolate* isolate,
                                                const std::string& plaintext) {
  // DCHECK(SafeStorage::electron_crypto_ready);
  std::string ciphertext;
  bool encrypted = OSCrypt::EncryptString(plaintext, &ciphertext);

  if (!encrypted) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while encrypting the text provided to "
        "safeStorage.encryptString.");
    return v8::Local<v8::Value>();
  }

  return node::Buffer::Copy(isolate, ciphertext.c_str(), ciphertext.size())
      .ToLocalChecked();
}

std::string SafeStorage::DecryptString(v8::Isolate* isolate,
                                       v8::Local<v8::Value> buffer) {
  if (!node::Buffer::HasInstance(buffer)) {
    LOG(ERROR) << "NOT A BUFFER!";
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Expected the first argument of decryptString() to be a buffer");
    return "";
  }

  const char* data = node::Buffer::Data(buffer);
  auto size = node::Buffer::Length(buffer);
  std::string ciphertext(data, size);
  LOG(ERROR) << "Stringified buffer!" << ciphertext;

  std::string plaintext;
  bool decrypted = OSCrypt::DecryptString(ciphertext, &plaintext);

  if (!decrypted) {
    LOG(ERROR) << "error while decrypting";
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while decrypting the ciphertext provided to "
        "safeStorage.decryptString.");
    return "";
  }
  LOG(ERROR) << "plaintext: decrypt success!" << plaintext;
  return plaintext;
}

gin::ObjectTemplateBuilder SafeStorage::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::ObjectTemplateBuilder(isolate)
      .SetMethod("isEncryptionAvailable", &SafeStorage::IsEncryptionAvailable)
      .SetMethod("encryptString", &SafeStorage::EncryptString)
      .SetMethod("decryptString", &SafeStorage::DecryptString)
      .SetMethod("getRawEncryptionKey", &OSCrypt::GetRawEncryptionKey);
}

const char* SafeStorage::GetTypeName() {
  return "SafeStorage";
}

// static
gin::Handle<SafeStorage> SafeStorage::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new SafeStorage());
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("safeStorage", electron::api::SafeStorage::Create(isolate));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_safe_storage, Initialize)
