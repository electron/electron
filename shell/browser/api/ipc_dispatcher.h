// Copyright (c) 2024 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_IPC_DISPATCHER_H_
#define ELECTRON_SHELL_BROWSER_API_IPC_DISPATCHER_H_

#include <string>

#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "gin/handle.h"
#include "shell/browser/api/message_port.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event.h"
#include "shell/common/gin_helper/reply_channel.h"
#include "shell/common/v8_util.h"

namespace electron {

// Handles dispatching IPCs to JS.
// See ipc-dispatch.ts for JS listeners.
template <typename T>
class IpcDispatcher {
 public:
  void Message(gin::Handle<gin_helper::internal::Event>& event,
               const std::string& channel,
               blink::CloneableMessage args) {
    TRACE_EVENT1("electron", "IpcDispatcher::Message", "channel", channel);
    emitter()->EmitWithoutEvent("-ipc-message", event, channel, args);
  }

  void Invoke(gin::Handle<gin_helper::internal::Event>& event,
              const std::string& channel,
              blink::CloneableMessage arguments,
              electron::mojom::ElectronApiIPC::InvokeCallback callback) {
    TRACE_EVENT1("electron", "IpcHelper::Invoke", "channel", channel);

    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    gin_helper::Dictionary dict(isolate, event.ToV8().As<v8::Object>());
    dict.Set("_replyChannel", gin_helper::internal::ReplyChannel::Create(
                                  isolate, std::move(callback)));

    emitter()->EmitWithoutEvent("-ipc-invoke", event, channel,
                                std::move(arguments));
  }

  void ReceivePostMessage(gin::Handle<gin_helper::internal::Event>& event,
                          const std::string& channel,
                          blink::TransferableMessage message) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    auto wrapped_ports =
        MessagePort::EntanglePorts(isolate, std::move(message.ports));
    v8::Local<v8::Value> message_value =
        electron::DeserializeV8Value(isolate, message);
    emitter()->EmitWithoutEvent("-ipc-ports", event, channel, message_value,
                                std::move(wrapped_ports));
  }

  void MessageSync(
      gin::Handle<gin_helper::internal::Event>& event,
      const std::string& channel,
      blink::CloneableMessage arguments,
      electron::mojom::ElectronApiIPC::MessageSyncCallback callback) {
    TRACE_EVENT1("electron", "IpcHelper::MessageSync", "channel", channel);

    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    gin_helper::Dictionary dict(isolate, event.ToV8().As<v8::Object>());
    dict.Set("_replyChannel", gin_helper::internal::ReplyChannel::Create(
                                  isolate, std::move(callback)));

    emitter()->EmitWithoutEvent("-ipc-message-sync", event, channel,
                                std::move(arguments));
  }

 private:
  inline T* emitter() {
    // T must inherit from gin_helper::EventEmitterMixin<T>
    return static_cast<T*>(this);
  }
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_IPC_DISPATCHER_H_
