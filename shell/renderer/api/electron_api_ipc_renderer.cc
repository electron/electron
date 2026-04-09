// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/worker_thread.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/function_template_extensions.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/wrappable.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_util.h"
#include "shell/renderer/preload_realm_context.h"
#include "shell/renderer/service_worker_data.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/web/modules/service_worker/web_service_worker_context_proxy.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_message_port_converter.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"  // nogncheck

using blink::WebLocalFrame;
using content::RenderFrame;

namespace {

const char kIPCMethodCalledAfterContextReleasedError[] =
    "IPC method called after context was released";

RenderFrame* GetCurrentRenderFrame() {
  WebLocalFrame* frame = WebLocalFrame::FrameForCurrentContext();
  if (!frame)
    return nullptr;

  return RenderFrame::FromWebFrame(frame);
}

// Thread identifier for the main renderer thread (as opposed to a service
// worker thread).
inline constexpr int kMainThreadId = 0;

bool IsWorkerThread() {
  return content::WorkerThread::GetCurrentId() != kMainThreadId;
}

template <typename T>
class IPCBase : public gin_helper::DeprecatedWrappable<T> {
 public:
  static gin::DeprecatedWrapperInfo kWrapperInfo;

  static gin_helper::Handle<T> Create(v8::Isolate* isolate) {
    return gin_helper::CreateHandle(isolate, new T(isolate));
  }

  void SendMessage(v8::Isolate* isolate,
                   gin_helper::ErrorThrower thrower,
                   bool internal,
                   const std::string& channel,
                   v8::Local<v8::Value> arguments) {
    if (!electron_ipc_remote_) {
      thrower.ThrowError(kIPCMethodCalledAfterContextReleasedError);
      return;
    }
    blink::CloneableMessage message;
    if (!electron::SerializeV8Value(isolate, arguments, &message)) {
      return;
    }
    electron_ipc_remote_->Message(internal, channel, std::move(message));
  }

  v8::Local<v8::Promise> Invoke(v8::Isolate* isolate,
                                gin_helper::ErrorThrower thrower,
                                bool internal,
                                const std::string& channel,
                                v8::Local<v8::Value> arguments) {
    if (!electron_ipc_remote_) {
      thrower.ThrowError(kIPCMethodCalledAfterContextReleasedError);
      return {};
    }
    blink::CloneableMessage message;
    if (!electron::SerializeV8Value(isolate, arguments, &message)) {
      return {};
    }
    gin_helper::Promise<blink::CloneableMessage> p(isolate);
    auto handle = p.GetHandle();

    electron_ipc_remote_->Invoke(
        internal, channel, std::move(message),
        base::BindOnce(
            [](gin_helper::Promise<blink::CloneableMessage> p,
               blink::CloneableMessage result) { p.Resolve(result); },
            std::move(p)));

    return handle;
  }

  void PostMessage(v8::Isolate* isolate,
                   gin_helper::ErrorThrower thrower,
                   const std::string& channel,
                   v8::Local<v8::Value> message_value,
                   std::optional<v8::Local<v8::Value>> transfer) {
    if (!electron_ipc_remote_) {
      thrower.ThrowError(kIPCMethodCalledAfterContextReleasedError);
      return;
    }
    blink::TransferableMessage transferable_message;
    if (!electron::SerializeV8Value(isolate, message_value,
                                    &transferable_message)) {
      // SerializeV8Value sets an exception.
      return;
    }

    std::vector<v8::Local<v8::Object>> transferables;
    if (transfer && !transfer.value()->IsUndefined()) {
      if (!gin::ConvertFromV8(isolate, *transfer, &transferables)) {
        thrower.ThrowTypeError("Invalid value for transfer");
        return;
      }
    }

    std::vector<blink::MessagePortChannel> ports;
    for (auto& transferable : transferables) {
      std::optional<blink::MessagePortChannel> port =
          blink::WebMessagePortConverter::
              DisentangleAndExtractMessagePortChannel(isolate, transferable);
      if (!port.has_value()) {
        thrower.ThrowTypeError("Invalid value for transfer");
        return;
      }
      ports.emplace_back(port.value());
    }

    transferable_message.ports = std::move(ports);
    electron_ipc_remote_->ReceivePostMessage(channel,
                                             std::move(transferable_message));
  }

  void SendToHost(v8::Isolate* isolate,
                  gin_helper::ErrorThrower thrower,
                  const std::string& channel,
                  v8::Local<v8::Value> arguments) {
    if (!electron_ipc_remote_) {
      thrower.ThrowError(kIPCMethodCalledAfterContextReleasedError);
      return;
    }
    blink::CloneableMessage message;
    if (!electron::SerializeV8Value(isolate, arguments, &message)) {
      return;
    }
    electron_ipc_remote_->MessageHost(channel, std::move(message));
  }

  v8::Local<v8::Value> SendSync(v8::Isolate* isolate,
                                gin_helper::ErrorThrower thrower,
                                bool internal,
                                const std::string& channel,
                                v8::Local<v8::Value> arguments) {
    if (!electron_ipc_remote_) {
      thrower.ThrowError(kIPCMethodCalledAfterContextReleasedError);
      return {};
    }
    blink::CloneableMessage message;
    if (!electron::SerializeV8Value(isolate, arguments, &message)) {
      return {};
    }

    blink::CloneableMessage result;
    electron_ipc_remote_->MessageSync(internal, channel, std::move(message),
                                      &result);
    return electron::DeserializeV8Value(isolate, result);
  }

  // gin_helper::Wrappable:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin_helper::DeprecatedWrappable<T>::GetObjectTemplateBuilder(isolate)
        .SetMethod("send", &T::SendMessage)
        .SetMethod("sendSync", &T::SendSync)
        .SetMethod("sendToHost", &T::SendToHost)
        .SetMethod("invoke", &T::Invoke)
        .SetMethod("postMessage", &T::PostMessage);
  }

 protected:
  mojo::AssociatedRemote<electron::mojom::ElectronApiIPC> electron_ipc_remote_;
};

class IPCRenderFrame : public IPCBase<IPCRenderFrame>,
                       private content::RenderFrameObserver {
 public:
  explicit IPCRenderFrame(v8::Isolate* isolate)
      : content::RenderFrameObserver(GetCurrentRenderFrame()) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    blink::ExecutionContext* execution_context =
        blink::ExecutionContext::From(context);

    if (execution_context->IsWindow()) {
      RenderFrame* render_frame = GetCurrentRenderFrame();
      DCHECK(render_frame);
      render_frame->GetRemoteAssociatedInterfaces()->GetInterface(
          &electron_ipc_remote_);
    } else {
      NOTREACHED();
    }

    weak_context_ =
        v8::Global<v8::Context>(isolate, isolate->GetCurrentContext());
    weak_context_.SetWeak();
  }

  void OnDestruct() override { electron_ipc_remote_.reset(); }

  void WillReleaseScriptContext(v8::Isolate* const isolate,
                                v8::Local<v8::Context> context,
                                int32_t world_id) override {
    if (weak_context_.IsEmpty() || weak_context_.Get(isolate) == context) {
      OnDestruct();
    }
  }

  const char* GetTypeName() override { return "IPCRenderFrame"; }

 private:
  v8::Global<v8::Context> weak_context_;
};

template <>
gin::DeprecatedWrapperInfo IPCBase<IPCRenderFrame>::kWrapperInfo = {
    gin::kEmbedderNativeGin};

class IPCServiceWorker : public IPCBase<IPCServiceWorker>,
                         public content::WorkerThread::Observer {
 public:
  explicit IPCServiceWorker(v8::Isolate* isolate) {
    DCHECK(IsWorkerThread());
    content::WorkerThread::AddObserver(this);

    electron::ServiceWorkerData* service_worker_data =
        electron::preload_realm::GetServiceWorkerData(
            isolate->GetCurrentContext());
    DCHECK(service_worker_data);
    service_worker_data->proxy()->GetRemoteAssociatedInterface(
        electron_ipc_remote_.BindNewEndpointAndPassReceiver());
  }

  void WillStopCurrentWorkerThread() override { electron_ipc_remote_.reset(); }

  const char* GetTypeName() override { return "IPCServiceWorker"; }
};

template <>
gin::DeprecatedWrapperInfo IPCBase<IPCServiceWorker>::kWrapperInfo = {
    gin::kEmbedderNativeGin};

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod("createForRenderFrame", &IPCRenderFrame::Create);
  dict.SetMethod("createForServiceWorker", &IPCServiceWorker::Create);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_renderer_ipc, Initialize)
