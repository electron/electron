// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_SERVICES_NODE_PARENT_PORT_H_
#define ELECTRON_SHELL_SERVICES_NODE_PARENT_PORT_H_

#include <memory>

#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/connector.h"
#include "mojo/public/cpp/bindings/message.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gc_plugin.h"
#include "shell/common/gin_helper/constructible.h"
#include "third_party/blink/public/common/messaging/message_port_descriptor.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
class ErrorThrower;
}  // namespace gin_helper

namespace v8 {
template <class T>
class Local;
class Value;
class Isolate;
}  // namespace v8

namespace electron {

// There is only a single instance of this class for the lifetime of a Utility
// Process. It is allocated on the V8 cppgc heap and kept alive for the entire
// process lifetime by a leaked cppgc::Persistent root, so it is never
// garbage collected.
class ParentPort final : public gin::Wrappable<ParentPort>,
                         public gin_helper::EventEmitterMixin<ParentPort>,
                         public gin_helper::Constructible<ParentPort>,
                         private mojo::MessageReceiver {
 public:
  static ParentPort* GetInstance();
  static ParentPort* Create(v8::Isolate* isolate);
  // gin_helper::Constructible; not constructible from JavaScript.
  static v8::Local<v8::Value> New(gin_helper::ErrorThrower thrower);
  static void FillObjectTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static const char* GetClassName() { return "ParentPort"; }

  ParentPort(const ParentPort&) = delete;
  ParentPort& operator=(const ParentPort&) = delete;

  ParentPort();
  ~ParentPort() override;
  void Initialize(blink::MessagePortDescriptor port);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;

  void Close();

 private:
  void PostMessage(gin::Arguments* args);
  void Start();
  void Pause();

  // mojo::MessageReceiver
  bool Accept(mojo::Message* mojo_message) override;

  bool connector_closed_ = false;
  GC_PLUGIN_IGNORE(
      "Context tracking of the connector is not needed in the utility "
      "process.")
  std::unique_ptr<mojo::Connector> connector_;
  blink::MessagePortDescriptor port_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_SERVICES_NODE_PARENT_PORT_H_
