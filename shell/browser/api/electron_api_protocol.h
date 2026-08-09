// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PROTOCOL_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PROTOCOL_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "gin/wrappable.h"
#include "shell/browser/net/electron_url_loader_factory.h"
#include "shell/common/gin_helper/constructible.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace electron {

class ProtocolRegistry;

namespace api {

std::vector<std::string>& GetStandardSchemes();
std::vector<std::string>& GetCodeCacheSchemes();

void AddServiceWorkerScheme(const std::string& scheme);

void RegisterSchemesAsPrivileged(gin_helper::ErrorThrower thrower,
                                 v8::Local<v8::Value> val);

// Protocol implementation based on network services.
class Protocol final : public gin::Wrappable<Protocol>,
                       public gin_helper::Constructible<Protocol> {
 public:
  static Protocol* Create(v8::Isolate* isolate,
                          ProtocolRegistry* protocol_registry);

  // gin_helper::Constructible
  static Protocol* New(gin_helper::ErrorThrower thrower);
  static void FillObjectTemplate(v8::Isolate* isolate,
                                 v8::Local<v8::ObjectTemplate> tmpl);
  static const char* GetClassName() { return "Protocol"; }

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;

  explicit Protocol(ProtocolRegistry* protocol_registry);
  ~Protocol() override;

 private:
  // Possible errors.
  enum class Error {
    kOK,  // no error
    kRegistered,
    kNotRegistered,
    kIntercepted,
    kNotIntercepted,
  };

  // Callback types.
  using CompletionCallback =
      base::RepeatingCallback<void(v8::Local<v8::Value>)>;

  [[nodiscard]] static std::string_view ErrorCodeToString(Error error);

  // JS APIs.
  Error RegisterProtocol(ProtocolType type,
                         const std::string& scheme,
                         const ProtocolHandler& handler);
  bool UnregisterProtocol(const std::string& scheme, gin::Arguments* args);
  bool IsProtocolRegistered(const std::string& scheme);

  Error InterceptProtocol(ProtocolType type,
                          const std::string& scheme,
                          const ProtocolHandler& handler);
  bool UninterceptProtocol(const std::string& scheme, gin::Arguments* args);
  bool IsProtocolIntercepted(const std::string& scheme);

  // Old async version of IsProtocolRegistered.
  v8::Local<v8::Promise> IsProtocolHandled(v8::Isolate* isolate,
                                           const std::string& scheme);

  // Helper for converting old registration APIs to new RegisterProtocol API.
  template <ProtocolType type>
  bool RegisterProtocolFor(const std::string& scheme,
                           const ProtocolHandler& handler,
                           gin::Arguments* args) {
    const auto result = RegisterProtocol(type, scheme, handler);
    HandleOptionalCallback(args, result);
    return result == Error::kOK;
  }
  template <ProtocolType type>
  bool InterceptProtocolFor(const std::string& scheme,
                            const ProtocolHandler& handler,
                            gin::Arguments* args) {
    const auto result = InterceptProtocol(type, scheme, handler);
    HandleOptionalCallback(args, result);
    return result == Error::kOK;
  }

  // Be compatible with old interface, which accepts optional callback.
  void HandleOptionalCallback(gin::Arguments* args, Error error);

  // Weak pointer; the lifetime of the ProtocolRegistry is guaranteed to be
  // longer than the lifetime of this JS interface.
  raw_ptr<ProtocolRegistry> protocol_registry_;
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PROTOCOL_H_
