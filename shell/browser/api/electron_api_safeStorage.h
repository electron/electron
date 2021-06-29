// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_SAFESTORAGE_H_
#define SHELL_BROWSER_API_ELECTRON_API_SAFESTORAGE_H_

#include <string>

#include "gin/wrappable.h"

namespace electron {

namespace api {

// TODO: am I using the right thingys here
class SafeStorage : public gin::Wrappable<SafeStorage>,
                    public gin_helper::Constructible<Tray>,
                    public gin_helper::CleanedUpAtExit,
                    public gin_helper::EventEmitterMixin<Cookies> {
 public:
  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  bool IsEncryptionAvailable();
  bool EncryptString(std::string& plaintext, std::string* ciphertext);
  bool DecryptString(std::string& ciphertext, std::string* plaintext);

  // TODO: private?
 private:
  SafeStorage(v8::Isolate* isolate);
  ~SafeStorage() override;
}

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_COOKIES_H_