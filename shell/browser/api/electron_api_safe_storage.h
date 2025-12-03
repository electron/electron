// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_

#include <string>

#include "build/build_config.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "shell/browser/browser_observer.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/wrappable.h"

namespace v8 {
class Context;
class Isolate;
class Object;
class Value;
template <class T>
class Local;
}  // namespace v8

namespace gin {
class ObjectTemplateBuilder;
}  // namespace gin

namespace gin_helper {
template <typename T>
class Handle;
}  // namespace gin_helper

namespace electron::api {

class SafeStorage final
    : public gin_helper::DeprecatedWrappable<SafeStorage>,
      public gin_helper::EventEmitterMixin<SafeStorage>,
      private BrowserObserver {
 public:
  static gin_helper::Handle<SafeStorage> Create(v8::Isolate* isolate);

  // gin_helper::Wrappable
  static gin::DeprecatedWrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // disable copy
  SafeStorage(const SafeStorage&) = delete;
  SafeStorage& operator=(const SafeStorage&) = delete;

 protected:
  explicit SafeStorage(v8::Isolate* isolate);
  ~SafeStorage() override;

 private:
  // BrowserObserver: 
  void OnFinishLaunching(base::Value::Dict launch_info) override;

  void OnOsCryptReady(os_crypt_async::Encryptor encryptor);

  bool IsEncryptionAvailable();

  void SetUsePasswordV10(bool use);

  v8::Local<v8::Value> EncryptString(v8::Isolate* isolate,
                                     const std::string& plaintext);

  std::string DecryptString(v8::Isolate* isolate,
                            v8::Local<v8::Value> buffer);

#if BUILDFLAG(IS_LINUX)
  std::string GetSelectedLinuxBackend();
#endif

  bool use_password_v10_ = false;

  bool is_available_ = false;

  std::optional<os_crypt_async::Encryptor> encryptor_;};

}  // namespace electron::api

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv);

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_
