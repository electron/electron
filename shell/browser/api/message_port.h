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
#include "shell/common/gin_helper/self_keep_alive.h"
#include "third_party/blink/public/common/messaging/message_port_channel.h"
#include "third_party/blink/public/common/messaging/message_port_descriptor.h"

namespace gin {
class Arguments;
class ObjectTemplateBuilder;
}  // namespace gin

namespace mojo {
class Connector;
}  // namespace mojo

namespace electron {

// A non-blink version of blink::MessagePort.
class MessagePort final : public gin::Wrappable<MessagePort>,
                          private mojo::MessageReceiver {
 public:
  ~MessagePort() override;
  static MessagePort* Create(v8::Isolate* isolate);

  MessagePort();

  void PostMessage(gin::Arguments* args);
  void Start();
  void Close();

  void Entangle(blink::MessagePortDescriptor port);
  void Entangle(blink::MessagePortChannel channel);

  blink::MessagePortChannel Disentangle();

  [[nodiscard]] bool IsEntangled() const;
  [[nodiscard]] bool IsNeutered() const;

  static std::vector<MessagePort*> EntanglePorts(
      v8::Isolate* isolate,
      std::vector<blink::MessagePortChannel> channels);

  static std::vector<blink::MessagePortChannel> DisentanglePorts(
      v8::Isolate* isolate,
      const std::vector<MessagePort*>& ports,
      bool* threw_exception);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor* visitor) const override;

  MessagePort(const MessagePort&) = delete;
  MessagePort& operator=(const MessagePort&) = delete;

 private:
  // Now that MessagePort lives on V8's unified cppgc heap it is, like blink's
  // MessagePort, a garbage-collected object. Blink additionally keeps such an
  // object alive while it has work outstanding by deriving from
  // ActiveScriptWrappable: a per-context ActiveScriptWrappableManager re-polls
  // every registered wrappable's HasPendingActivity() at each GC and roots the
  // active ones for that cycle. See
  // https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/platform/bindings/active_script_wrappable_manager.cc;l=35;drc=b892cf58e162a8f66cd76d7472f129fe0fb6a7d1
  // That machinery is part of blink's ScriptWrappable/ThreadState bindings and
  // isn't available to a gin::Wrappable, so instead of being polled we drive
  // the equivalent manually: we hold a SelfKeepAlive root and, whenever the
  // value of HasPendingActivity() changes, set or clear |keep_alive_| as
  // appropriate.
  bool HasPendingActivity() const;

  // mojo::MessageReceiver
  bool Accept(mojo::Message* mojo_message) override;

  std::unique_ptr<mojo::Connector> connector_;
  bool started_ = false;
  bool closed_ = false;

  // The internal port owned by this class. The handle itself is moved into the
  // |connector_| while entangled.
  blink::MessagePortDescriptor port_;

  gin_helper::SelfKeepAlive<MessagePort> keep_alive_{nullptr};

  gin::WeakCellFactory<MessagePort> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_MESSAGE_PORT_H_
