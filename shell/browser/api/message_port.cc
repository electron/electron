// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/message_port.h"

#include <string>
#include <utility>

#include "base/containers/to_vector.h"
#include "base/task/single_thread_task_runner.h"
#include "gin/arguments.h"
#include "gin/data_object_builder.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/wrappable.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_util.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"
#include "third_party/blink/public/common/messaging/transferable_message.h"
#include "third_party/blink/public/common/messaging/transferable_message_mojom_traits.h"
#include "third_party/blink/public/mojom/messaging/transferable_message.mojom.h"

namespace electron {

gin::WrapperInfo MessagePort::kWrapperInfo = {gin::kEmbedderNativeGin};

MessagePort::MessagePort() = default;
MessagePort::~MessagePort() {
  if (!IsNeutered()) {
    // Disentangle before teardown. The MessagePortDescriptor will blow up if it
    // hasn't had its underlying handle returned to it before teardown.
    Disentangle();
  }
}

// static
gin::Handle<MessagePort> MessagePort::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new MessagePort());
}

bool MessagePort::IsEntangled() const {
  return !closed_ && !IsNeutered();
}

bool MessagePort::IsNeutered() const {
  return !connector_ || !connector_->is_valid();
}

void MessagePort::PostMessage(gin::Arguments* args) {
  if (!IsEntangled())
    return;
  DCHECK(!IsNeutered());

  blink::TransferableMessage transferable_message;
  gin_helper::ErrorThrower thrower(args->isolate());

  // |message| is any value that can be serialized to StructuredClone.
  v8::Local<v8::Value> message_value;
  if (args->GetNext(&message_value)) {
    if (!electron::SerializeV8Value(args->isolate(), message_value,
                                    &transferable_message)) {
      // SerializeV8Value sets an exception.
      return;
    }
  }

  v8::Local<v8::Value> transferables;
  std::vector<gin::Handle<MessagePort>> wrapped_ports;
  if (args->GetNext(&transferables)) {
    std::vector<v8::Local<v8::Value>> wrapped_port_values;
    if (!gin::ConvertFromV8(args->isolate(), transferables,
                            &wrapped_port_values)) {
      thrower.ThrowTypeError("transferables must be an array of MessagePorts");
      return;
    }

    for (unsigned i = 0; i < wrapped_port_values.size(); ++i) {
      if (!gin_helper::IsValidWrappable(wrapped_port_values[i],
                                        &MessagePort::kWrapperInfo)) {
        thrower.ThrowTypeError("Port at index " + base::NumberToString(i) +
                               " is not a valid port");
        return;
      }
    }

    if (!gin::ConvertFromV8(args->isolate(), transferables, &wrapped_ports)) {
      thrower.ThrowTypeError("Passed an invalid MessagePort");
      return;
    }
  }

  // Make sure we aren't connected to any of the passed-in ports.
  for (unsigned i = 0; i < wrapped_ports.size(); ++i) {
    if (wrapped_ports[i].get() == this) {
      thrower.ThrowError("Port at index " + base::NumberToString(i) +
                         " contains the source port.");
      return;
    }
  }

  bool threw_exception = false;
  transferable_message.ports = MessagePort::DisentanglePorts(
      args->isolate(), wrapped_ports, &threw_exception);
  if (threw_exception)
    return;

  mojo::Message mojo_message = blink::mojom::TransferableMessage::WrapAsMessage(
      std::move(transferable_message));
  connector_->Accept(&mojo_message);
}

void MessagePort::Start() {
  if (!IsEntangled())
    return;

  if (started_)
    return;

  started_ = true;
  if (HasPendingActivity())
    Pin();
  connector_->ResumeIncomingMethodCallProcessing();
}

void MessagePort::Close() {
  if (closed_)
    return;
  if (!IsNeutered()) {
    Disentangle().ReleaseHandle();
    blink::MessagePortDescriptorPair pipe;
    Entangle(pipe.TakePort0());
  }
  closed_ = true;
  if (!HasPendingActivity())
    Unpin();

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> self;
  if (GetWrapper(isolate).ToLocal(&self))
    gin_helper::EmitEvent(isolate, self, "close");
}

void MessagePort::Entangle(blink::MessagePortDescriptor port) {
  DCHECK(port.IsValid());
  DCHECK(!connector_);
  port_ = std::move(port);
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  connector_ = std::make_unique<mojo::Connector>(
      port_.TakeHandleToEntangleWithEmbedder(),
      mojo::Connector::SINGLE_THREADED_SEND,
      base::SingleThreadTaskRunner::GetCurrentDefault());
  connector_->PauseIncomingMethodCallProcessing();
  connector_->set_incoming_receiver(this);
  connector_->set_connection_error_handler(
      base::BindOnce(&MessagePort::Close, weak_factory_.GetWeakPtr()));
  if (HasPendingActivity())
    Pin();
}

void MessagePort::Entangle(blink::MessagePortChannel channel) {
  Entangle(channel.ReleaseHandle());
}

blink::MessagePortChannel MessagePort::Disentangle() {
  DCHECK(!IsNeutered());
  port_.GiveDisentangledHandle(connector_->PassMessagePipe());
  connector_ = nullptr;
  if (!HasPendingActivity())
    Unpin();
  return blink::MessagePortChannel(std::move(port_));
}

bool MessagePort::HasPendingActivity() const {
  // The spec says that entangled message ports should always be treated as if
  // they have a strong reference.
  // We'll also stipulate that the queue needs to be open (if the app drops its
  // reference to the port before start()-ing it, then it's not really entangled
  // as it's unreachable).
  return started_ && IsEntangled();
}

// static
std::vector<gin::Handle<MessagePort>> MessagePort::EntanglePorts(
    v8::Isolate* isolate,
    std::vector<blink::MessagePortChannel> channels) {
  std::vector<gin::Handle<MessagePort>> wrapped_ports;
  for (auto& port : channels) {
    auto wrapped_port = MessagePort::Create(isolate);
    wrapped_port->Entangle(std::move(port));
    wrapped_ports.emplace_back(wrapped_port);
  }
  return wrapped_ports;
}

// static
std::vector<blink::MessagePortChannel> MessagePort::DisentanglePorts(
    v8::Isolate* isolate,
    const std::vector<gin::Handle<MessagePort>>& ports,
    bool* threw_exception) {
  if (ports.empty())
    return {};

  absl::flat_hash_set<MessagePort*> visited;
  visited.reserve(ports.size());

  // Walk the incoming array - if there are any duplicate ports, or null ports
  // or cloned ports, throw an error (per section 8.3.3 of the HTML5 spec).
  for (unsigned i = 0; i < ports.size(); ++i) {
    auto* port = ports[i].get();
    if (!port || port->IsNeutered() || visited.contains(port)) {
      std::string type;
      if (!port)
        type = "null";
      else if (port->IsNeutered())
        type = "already neutered";
      else
        type = "a duplicate";
      gin_helper::ErrorThrower(isolate).ThrowError(
          "Port at index " + base::NumberToString(i) + " is " + type + ".");
      *threw_exception = true;
      return {};
    }
    visited.insert(port);
  }

  // Passed-in ports passed validity checks, so we can disentangle them.
  return base::ToVector(ports, [](auto& port) { return port->Disentangle(); });
}

void MessagePort::Pin() {
  if (!pinned_.IsEmpty())
    return;
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Value> self;
  if (GetWrapper(isolate).ToLocal(&self)) {
    pinned_.Reset(isolate, self);
  }
}

void MessagePort::Unpin() {
  pinned_.Reset();
}

bool MessagePort::Accept(mojo::Message* mojo_message) {
  blink::TransferableMessage message;
  if (!blink::mojom::TransferableMessage::DeserializeFromMessage(
          std::move(*mojo_message), &message)) {
    return false;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);

  auto ports = EntanglePorts(isolate, std::move(message.ports));

  v8::Local<v8::Value> message_value = DeserializeV8Value(isolate, message);

  v8::Local<v8::Object> self;
  if (!GetWrapper(isolate).ToLocal(&self))
    return false;

  auto event = gin::DataObjectBuilder(isolate)
                   .Set("data", message_value)
                   .Set("ports", ports)
                   .Build();
  gin_helper::EmitEvent(isolate, self, "message", event);
  return true;
}

gin::ObjectTemplateBuilder MessagePort::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<MessagePort>::GetObjectTemplateBuilder(isolate)
      .SetMethod("postMessage", &MessagePort::PostMessage)
      .SetMethod("start", &MessagePort::Start)
      .SetMethod("close", &MessagePort::Close);
}

const char* MessagePort::GetTypeName() {
  return "MessagePort";
}

void MessagePort::WillBeDestroyed() {
  ClearWeak();
}

}  // namespace electron

namespace {

using electron::MessagePort;

v8::Local<v8::Value> CreatePair(v8::Isolate* isolate) {
  auto port1 = MessagePort::Create(isolate);
  auto port2 = MessagePort::Create(isolate);
  blink::MessagePortDescriptorPair pipe;
  port1->Entangle(pipe.TakePort0());
  port2->Entangle(pipe.TakePort1());
  return gin::DataObjectBuilder(isolate)
      .Set("port1", port1)
      .Set("port2", port2)
      .Build();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("createPair", &CreatePair);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_message_port, Initialize)
