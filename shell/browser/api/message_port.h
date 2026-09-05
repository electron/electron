// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_MESSAGE_PORT_H_
#define ELECTRON_SHELL_BROWSER_API_MESSAGE_PORT_H_

#include <memory>
#include <vector>

#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/message.h"
#include "shell/common/gc_plugin.h"
#include "shell/common/gin_helper/self_keep_alive.h"
#include "third_party/blink/public/common/messaging/message_port_channel.h"
#include "third_party/blink/public/common/messaging/message_port_descriptor.h"
#include "v8/include/v8-local-handle.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace mojo {
class Connector;
}  // namespace mojo

namespace electron {

// A non-blink version of blink::MessagePort.
class MessagePort final : public gin::Wrappable<MessagePort>,
                          private mojo::MessageReceiver {
 public:
  MessagePort();
  ~MessagePort() override;
  static MessagePort* Create(v8::Isolate* isolate);

  void PostMessage(gin::Arguments* args);
  void Start();
  void Close();

  void Entangle(blink::MessagePortDescriptor port);
  void Entangle(blink::MessagePortChannel channel);

  blink::MessagePortChannel Disentangle();

  [[nodiscard]] bool IsEntangled() const;
  [[nodiscard]] bool IsNeutered() const;

  static bool EntanglePorts(v8::Isolate* isolate,
                            std::vector<blink::MessagePortChannel> channels,
                            v8::LocalVector<v8::Value>* wrapped_ports);

  static std::vector<blink::MessagePortChannel> DisentanglePorts(
      v8::Isolate* isolate,
      const v8::LocalVector<v8::Value>& ports,
      bool* threw_exception,
      MessagePort* source_port = nullptr);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  static const char* GetClassName() { return "MessagePort"; }
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor* visitor) const override;

 private:
  // Started, entangled ports have pending activity and must stay alive even
  // after their JavaScript wrapper becomes unreachable.
  bool HasPendingActivity() const;
  void Pin();
  void Unpin();

  // mojo::MessageReceiver
  bool Accept(mojo::Message* mojo_message) override;

  GC_PLUGIN_IGNORE("The connector is owned by and cannot outlive MessagePort.")
  std::unique_ptr<mojo::Connector> connector_;
  bool started_ = false;
  bool closed_ = false;

  gin_helper::SelfKeepAlive<MessagePort> keep_alive_{nullptr};

  // The internal port owned by this class. The handle itself is moved into the
  // |connector_| while entangled.
  blink::MessagePortDescriptor port_;

  gin::WeakCellFactory<MessagePort> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_MESSAGE_PORT_H_
