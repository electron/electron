#include "shell/browser/api/electron_api_safeStorage.h"

#include "components/os_crypt/os_crypt.h"

namespace electron {

namespace api {

gin::WrapperInfo SafeStorage::kWrapperInfo = {gin::kEmbedderNativeGin};

SafeStorage::SafeStorage(v8::Isolate* isolate){};
SafeStorage::~SafeStorage() = default;

bool SafeStorage::IsEncryptionAvailable() {
  return OSCrypt::IsEncryptionAvailable();
}
// TODO: are all the pointers and dereferences right...
// should i be passing in *plaintext, &ciphertext
bool SafeStorage::EncryptString(std::string& plaintext,
                                std::string* ciphertext) {
  return OSCrypt::EncryptString(plaintext, ciphertext);
}

bool SafeStorage::DecryptString(std::string& ciphertext,
                                std::string* plaintext) {
  return OSCrypt::DecryptString(ciphertext, plaintext);
}

gin::ObjectTemplateBuilder SafeStorage::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<SafeStorage>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("encryptString", &SafeStorage::EncryptString)
      .SetMethod("decryptString", &SafeStorage::DecryptString)
      .SetMethod("isEncryptionAvailable", &SafeStorage::IsEncryptionAvailable);
}

}  // namespace api

}  // namespace electron