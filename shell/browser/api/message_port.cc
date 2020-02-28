// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/message_port.h"

#include "gin/arguments.h"
#include "gin/data_object_builder.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_value_serializer.h"
#include "third_party/blink/public/common/messaging/transferable_message.h"
#include "third_party/blink/public/common/messaging/transferable_message_mojom_traits.h"
#include "third_party/blink/public/mojom/messaging/transferable_message.mojom.h"

namespace electron {

gin::WrapperInfo MessagePort::kWrapperInfo = {gin::kEmbedderNativeGin};

MessagePort::MessagePort() = default;
MessagePort::~MessagePort() = default;

// static
gin::Handle<MessagePort> MessagePort::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new MessagePort());
}

void MessagePort::PostMessage(gin::Arguments* args) {
  if (!IsEntangled())
    return;
  DCHECK(!IsNeutered());

  blink::TransferableMessage transferable_message;

  v8::Local<v8::Value> message_value;
  if (!args->GetNext(&message_value)) {
    args->ThrowTypeError("Expected at least one argument to postMessage");
    return;
  }

  electron::SerializeV8Value(args->isolate(), message_value,
                             &transferable_message);

  v8::Local<v8::Value> transferables;
  std::vector<gin::Handle<MessagePort>> wrapped_ports;
  if (args->GetNext(&transferables)) {
    if (!gin::ConvertFromV8(args->isolate(), transferables, &wrapped_ports)) {
      args->ThrowError();
      return;
    }
  }
  transferable_message.ports =
      MessagePort::DisentanglePorts(args->isolate(), wrapped_ports);
  // TODO: check for exception

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
  connector_->ResumeIncomingMethodCallProcessing();
}

void MessagePort::Close() {
  if (closed_)
    return;
  if (!IsNeutered()) {
    connector_ = nullptr;
    Entangle(mojo::MessagePipe().handle0);
  }
  closed_ = true;
}

void MessagePort::Entangle(mojo::ScopedMessagePipeHandle handle) {
  DCHECK(handle.is_valid());
  DCHECK(!connector_);
  connector_ = std::make_unique<mojo::Connector>(
      std::move(handle), mojo::Connector::SINGLE_THREADED_SEND,
      base::ThreadTaskRunnerHandle::Get());
  connector_->PauseIncomingMethodCallProcessing();
  connector_->set_incoming_receiver(this);
  connector_->set_connection_error_handler(
      base::Bind(&MessagePort::Close, weak_factory_.GetWeakPtr()));
}

void MessagePort::Entangle(blink::MessagePortChannel channel) {
  Entangle(channel.ReleaseHandle());
}

blink::MessagePortChannel MessagePort::Disentangle() {
  DCHECK(!IsNeutered());
  auto result = blink::MessagePortChannel(connector_->PassMessagePipe());
  connector_ = nullptr;
  return result;
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
    const std::vector<gin::Handle<MessagePort>>& ports) {
  if (!ports.size())
    return std::vector<blink::MessagePortChannel>();

  std::unordered_set<MessagePort*> visited;

  // Walk the incoming array - if there are any duplicate ports, or null ports
  // or cloned ports, throw an error (per section 8.3.3 of the HTML5 spec).
  for (unsigned i = 0; i < ports.size(); ++i) {
    auto* port = ports[i].get();
    if (!port || port->IsNeutered() || visited.find(port) != visited.end()) {
      std::string type;
      if (!port)
        type = "null";
      else if (port->IsNeutered())
        type = "already neutered";
      else
        type = "a duplicate";
      /*
      exception_state.ThrowDOMException(
          DOMExceptionCode::kDataCloneError,
          "Port at index " + String::Number(i) + " is " + type + ".");
          */
      return std::vector<blink::MessagePortChannel>();
    }
    visited.insert(port);
  }

  // Passed-in ports passed validity checks, so we can disentangle them.
  std::vector<blink::MessagePortChannel> channels;
  channels.reserve(ports.size());
  for (unsigned i = 0; i < ports.size(); ++i)
    channels.push_back(ports[i]->Disentangle());
  return channels;
}

bool MessagePort::Accept(mojo::Message* mojo_message) {
  blink::TransferableMessage message;
  if (!blink::mojom::TransferableMessage::DeserializeFromMessage(
          std::move(*mojo_message), &message)) {
    return false;
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope scope(isolate);

  auto ports = EntanglePorts(isolate, std::move(message.ports));

  v8::Local<v8::Value> message_value = DeserializeV8Value(isolate, message);

  v8::Local<v8::Object> self;
  if (!GetWrapper(isolate).ToLocal(&self))
    return false;

  // TODO: inherit from the "Event" object..?
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

}  // namespace electron

namespace {

using electron::MessagePort;

v8::Local<v8::Value> CreatePair(v8::Isolate* isolate) {
  auto port1 = MessagePort::Create(isolate);
  auto port2 = MessagePort::Create(isolate);
  mojo::MessagePipe pipe;
  port1->Entangle(std::move(pipe.handle0));
  port2->Entangle(std::move(pipe.handle1));
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

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_message_port, Initialize)
