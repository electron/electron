// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_SERVICES_NODE_PARENT_PORT_H_
#define ELECTRON_SHELL_SERVICES_NODE_PARENT_PORT_H_

#include <memory>

#include "mojo/public/cpp/bindings/connector.h"
#include "mojo/public/cpp/bindings/message.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/gin_helper/wrappable.h"
#include "third_party/blink/public/common/messaging/message_port_descriptor.h"

namespace v8 {
template <class T>
class Local;
class Value;
class Isolate;
}  // namespace v8

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
template <typename T>
class Handle;
}  // namespace gin_helper

namespace electron {

// There is only a single instance of this class
// for the lifetime of a Utility Process which
// also means that GC lifecycle is ignored by this class.
class ParentPort final : public gin_helper::DeprecatedWrappable<ParentPort>,
                         public gin_helper::CleanedUpAtExit,
                         private mojo::MessageReceiver {
 public:
  static ParentPort* GetInstance();
  static gin_helper::Handle<ParentPort> Create(v8::Isolate* isolate);

  ParentPort(const ParentPort&) = delete;
  ParentPort& operator=(const ParentPort&) = delete;

  ParentPort();
  ~ParentPort() override;
  void Initialize(blink::MessagePortDescriptor port);

  // gin_helper::Wrappable
  static gin::DeprecatedWrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  void Close();

 private:
  void PostMessage(v8::Local<v8::Value> message_value);
  void Start();
  void Pause();

  // mojo::MessageReceiver
  bool Accept(mojo::Message* mojo_message) override;

  bool connector_closed_ = false;
  std::unique_ptr<mojo::Connector> connector_;
  blink::MessagePortDescriptor port_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_SERVICES_NODE_PARENT_PORT_H_
