// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_
#define SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_

#include <string>

#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"

namespace electron {

namespace api {

class SafeStorage : public gin::Wrappable<SafeStorage>,
                    public gin_helper::CleanedUpAtExit {
 public:
  static gin::Handle<SafeStorage> Create(v8::Isolate* isolate);
  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  bool IsEncryptionAvailable();
  v8::Local<v8::Value> EncryptString(v8::Isolate* isolate,
                                     const std::string& plaintext);
  std::string DecryptString(v8::Isolate* isolate,
                            v8::Local<v8::Value> ciphertext);
  // Used in a DCHECK to validate that our assumption that the network context
  // manager has initialized before app ready holds true. Only used in the
  // testing build
#if DCHECK_IS_ON()
  static bool electron_crypto_ready;
#endif

 private:
  SafeStorage(v8::Isolate* isolate);
  ~SafeStorage() override;
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_
