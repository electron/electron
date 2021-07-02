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

gin::WrapperInfo SafeStorage::kWrapperInfo = {gin::kEmbedderNativeGin};

electron::api::SafeStorage::SafeStorage(v8::Isolate* isolate) {}
electron::api::SafeStorage::~SafeStorage() = default;

bool SafeStorage::IsEncryptionAvailable() {
  return OSCrypt::IsEncryptionAvailable();
}

std::string SafeStorage::EncryptString(v8::Isolate* isolate,
                                       const std::string& plaintext) {
  std::string ciphertext;
  bool encrypted = OSCrypt::EncryptString(plaintext, &ciphertext);

  if (!encrypted) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while decrypting the key.");
  }

  return ciphertext;
}

std::string SafeStorage::DecryptString(v8::Isolate* isolate,
                                       const std::string& ciphertext) {
  std::string plaintext;
  bool encrypted = OSCrypt::DecryptString(ciphertext, &plaintext);

  if (!encrypted) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Error while decrypting the key.");
  }

  return plaintext;
}

gin::ObjectTemplateBuilder SafeStorage::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<SafeStorage>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("isEncryptionAvailable", &SafeStorage::IsEncryptionAvailable)
      .SetMethod("encryptString", &SafeStorage::EncryptString)
      .SetMethod("decryptString", &SafeStorage::DecryptString);
}

const char* SafeStorage::GetTypeName() {
  return "SafeStorage";
}

// static
gin::Handle<SafeStorage> SafeStorage::Create(v8::Isolate* isolate) {
  // CHECK(Browser::Get()->is_ready());
  return gin::CreateHandle(isolate, new SafeStorage(isolate));
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