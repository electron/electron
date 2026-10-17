// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <optional>
#include <string>

#include "base/strings/strcat.h"

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/worker_thread.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gc_plugin.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/serialized_value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/function_template_extensions.h"
#include "shell/common/gin_helper/node_event_emitter.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "shell/common/serialized_value.h"
#include "shell/common/v8_util.h"
#include "shell/renderer/preload_realm_context.h"
#include "shell/renderer/service_worker_data.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/web/modules/service_worker/web_service_worker_context_proxy.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_message_port_converter.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"  // nogncheck
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/prefinalizer.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-template.h"

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
class IPCBase : public gin::Wrappable<T> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static T* Create(v8::Isolate* isolate) {
    return cppgc::MakeGarbageCollected<T>(
        isolate->GetCppHeap()->GetAllocationHandle(), isolate);
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
    electron::SerializedValue message;
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
    electron::SerializedValue message;
    if (!electron::SerializeV8Value(isolate, arguments, &message)) {
      return {};
    }
    gin_helper::Promise<electron::SerializedValue> p(isolate);
    auto handle = p.GetHandle();

    electron_ipc_remote_->Invoke(
        internal, channel, std::move(message),
        base::BindOnce(
            [](gin_helper::Promise<electron::SerializedValue> p,
               electron::SerializedValue result) { p.Resolve(result); },
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
    electron::SerializedValue message;
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
    electron::SerializedValue message;
    if (!electron::SerializeV8Value(isolate, arguments, &message)) {
      return {};
    }

    electron::SerializedValue result;
    electron_ipc_remote_->MessageSync(internal, channel, std::move(message),
                                      &result);
    return electron::DeserializeV8Value(isolate, result);
  }

  // gin::Wrappable:
  const gin::WrapperInfo* wrapper_info() const override {
    return &kWrapperInfo;
  }

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::Wrappable<T>::GetObjectTemplateBuilder(isolate)
        .SetMethod("send", &T::SendMessage)
        .SetMethod("sendSync", &T::SendSync)
        .SetMethod("sendToHost", &T::SendToHost)
        .SetMethod("invoke", &T::Invoke)
        .SetMethod("postMessage", &T::PostMessage);
  }

 protected:
  GC_PLUGIN_IGNORE("Renderer IPC remotes do not need GC tracing.")
  mojo::AssociatedRemote<electron::mojom::ElectronApiIPC> electron_ipc_remote_;
};

class IPCRenderFrame final : public IPCBase<IPCRenderFrame>,
                             private content::RenderFrameObserver {
  CPPGC_USING_PRE_FINALIZER(IPCRenderFrame, Dispose);

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

  // Deregister from the RenderFrame's observer list before cppgc reclaims this
  // object.
  void Dispose() { content::RenderFrameObserver::Dispose(); }

  void WillReleaseScriptContext(v8::Isolate* const isolate,
                                v8::Local<v8::Context> context,
                                int32_t world_id) override {
    if (weak_context_.IsEmpty() || weak_context_.Get(isolate) == context) {
      OnDestruct();
    }
  }

  const char* GetHumanReadableName() const override {
    return "Electron / IPCRenderFrame";
  }

 private:
  v8::Global<v8::Context> weak_context_;
};

template <>
gin::WrapperInfo IPCBase<IPCRenderFrame>::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronIPCRenderFrame);

class IPCServiceWorker final : public IPCBase<IPCServiceWorker>,
                               public content::WorkerThread::Observer {
  CPPGC_USING_PRE_FINALIZER(IPCServiceWorker, Dispose);

 public:
  explicit IPCServiceWorker(v8::Isolate* isolate) {
    DCHECK(IsWorkerThread());
    content::WorkerThread::AddObserver(this);
    observing_worker_thread_ = true;

    electron::ServiceWorkerData* service_worker_data =
        electron::preload_realm::GetServiceWorkerData(
            isolate->GetCurrentContext());
    DCHECK(service_worker_data);
    service_worker_data->proxy()->GetRemoteAssociatedInterface(
        electron_ipc_remote_.BindNewEndpointAndPassReceiver());
  }

  // Deregister from the worker thread's observer list before cppgc reclaims
  // this object, otherwise thread shutdown would notify a swept observer.
  void Dispose() {
    if (observing_worker_thread_) {
      observing_worker_thread_ = false;
      content::WorkerThread::RemoveObserver(this);
    }
  }

  void WillStopCurrentWorkerThread() override {
    // The per-thread observer list is destroyed right after this returns, so
    // there is nothing left to deregister from in Dispose().
    observing_worker_thread_ = false;
    electron_ipc_remote_.reset();
  }

  const char* GetHumanReadableName() const override {
    return "Electron / IPCServiceWorker";
  }

 private:
  bool observing_worker_thread_ = false;
};

template <>
gin::WrapperInfo IPCBase<IPCServiceWorker>::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronIPCServiceWorker);

// The `ipcRenderer` / `ipcRendererInternal` objects: native EventEmitters whose
// send/invoke/... methods talk to an IPC transport (`T`) directly. Each method
// carries the transport's wrapper and the `internal` flag in its data object,
// so the functions work unbound (`const { send } = ipcRenderer`) as the
// JavaScript implementation's closures did.
enum MethodData { kTransport, kInternal, kMethodDataCount };

template <typename T>
struct EmitterMethods {
  static bool Unpack(const v8::FunctionCallbackInfo<v8::Value>& info,
                     const char* method,
                     T** transport,
                     bool* internal,
                     std::string* channel) {
    v8::Isolate* isolate = info.GetIsolate();
    v8::Local<v8::Object> data = info.Data().As<v8::Object>();
    *internal = data->GetInternalField(kInternal).As<v8::Value>()->IsTrue();
    if (!gin::ConvertFromV8(isolate,
                            data->GetInternalField(kTransport).As<v8::Value>(),
                            transport) ||
        !*transport) {
      gin_helper::ErrorThrower(isolate).ThrowError(
          kIPCMethodCalledAfterContextReleasedError);
      return false;
    }
    if (info.Length() < 1 || !gin::ConvertFromV8(isolate, info[0], channel)) {
      gin_helper::ErrorThrower(isolate).ThrowTypeError(
          base::StrCat({"Error processing argument at index 0 of ", method,
                        ", conversion failure: channel must be a string"}));
      return false;
    }
    return true;
  }

  // info[start..] as an array, for the transport's `args` parameter.
  static v8::Local<v8::Value> Rest(
      const v8::FunctionCallbackInfo<v8::Value>& info,
      int start) {
    v8::Isolate* isolate = info.GetIsolate();
    v8::LocalVector<v8::Value> rest(isolate);
    for (int i = start; i < info.Length(); ++i)
      rest.push_back(info[i]);
    return v8::Array::New(isolate, rest.data(), rest.size());
  }

  static void Send(const v8::FunctionCallbackInfo<v8::Value>& info) {
    T* transport;
    bool internal;
    std::string channel;
    if (!Unpack(info, "send", &transport, &internal, &channel))
      return;
    v8::Isolate* isolate = info.GetIsolate();
    transport->SendMessage(isolate, gin_helper::ErrorThrower(isolate), internal,
                           channel, Rest(info, 1));
  }

  static void SendSync(const v8::FunctionCallbackInfo<v8::Value>& info) {
    T* transport;
    bool internal;
    std::string channel;
    if (!Unpack(info, "sendSync", &transport, &internal, &channel))
      return;
    v8::Isolate* isolate = info.GetIsolate();
    v8::Local<v8::Value> result =
        transport->SendSync(isolate, gin_helper::ErrorThrower(isolate),
                            internal, channel, Rest(info, 1));
    if (!result.IsEmpty())
      info.GetReturnValue().Set(result);
  }

  static void SendToHost(const v8::FunctionCallbackInfo<v8::Value>& info) {
    T* transport;
    bool internal;
    std::string channel;
    if (!Unpack(info, "sendToHost", &transport, &internal, &channel))
      return;
    v8::Isolate* isolate = info.GetIsolate();
    transport->SendToHost(isolate, gin_helper::ErrorThrower(isolate), channel,
                          Rest(info, 1));
  }

  static void PostMessage(const v8::FunctionCallbackInfo<v8::Value>& info) {
    T* transport;
    bool internal;
    std::string channel;
    if (!Unpack(info, "postMessage", &transport, &internal, &channel))
      return;
    v8::Isolate* isolate = info.GetIsolate();
    std::optional<v8::Local<v8::Value>> transfer;
    if (info.Length() > 2)
      transfer = info[2];
    transport->PostMessage(isolate, gin_helper::ErrorThrower(isolate), channel,
                           info[1], transfer);
  }

  // invoke() resolves with the handler's return value or rejects with the
  // error the browser reported for `channel`.
  static void Invoke(const v8::FunctionCallbackInfo<v8::Value>& info) {
    T* transport;
    bool internal;
    std::string channel;
    if (!Unpack(info, "invoke", &transport, &internal, &channel))
      return;
    v8::Isolate* isolate = info.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Promise> reply =
        transport->Invoke(isolate, gin_helper::ErrorThrower(isolate), internal,
                          channel, Rest(info, 1));
    if (reply.IsEmpty())
      return;
    v8::Local<v8::Function> unwrap;
    v8::Local<v8::Promise> result;
    if (v8::Function::New(context, UnwrapInvokeResult, info[0], 1,
                          v8::ConstructorBehavior::kThrow)
            .ToLocal(&unwrap) &&
        reply->Then(context, unwrap).ToLocal(&result)) {
      info.GetReturnValue().Set(result);
    }
  }

  static void UnwrapInvokeResult(
      const v8::FunctionCallbackInfo<v8::Value>& info) {
    v8::Isolate* isolate = info.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> error, result;
    if (info.Length() < 1 || !info[0]->IsObject() ||
        !info[0]
             .As<v8::Object>()
             ->Get(context, gin::StringToSymbol(isolate, "error"))
             .ToLocal(&error) ||
        !info[0]
             .As<v8::Object>()
             ->Get(context, gin::StringToSymbol(isolate, "result"))
             .ToLocal(&result)) {
      return;
    }
    if (error->BooleanValue(isolate)) {
      v8::Local<v8::String> error_string;
      if (!error->ToString(context).ToLocal(&error_string))
        return;
      isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
          isolate, base::StrCat({"Error invoking remote method '",
                                 gin::V8ToString(isolate, info.Data()), "': ",
                                 gin::V8ToString(isolate, error_string)}))));
      return;
    }
    info.GetReturnValue().Set(result);
  }
};

template <typename T>
v8::Local<v8::Object> CreateEmitter(v8::Local<v8::Context> context,
                                    v8::Local<v8::Value> transport,
                                    bool internal) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::ObjectTemplate> data_template =
      v8::ObjectTemplate::New(isolate);
  data_template->SetInternalFieldCount(kMethodDataCount);
  v8::Local<v8::Object> data =
      data_template->NewInstance(context).ToLocalChecked();
  data->SetInternalField(kTransport, transport);
  data->SetInternalField(kInternal, v8::Boolean::New(isolate, internal));

  // emitter -> { send, invoke, ... } -> EventEmitter.prototype, mirroring the
  // old `class IpcRenderer extends EventEmitter` so the methods stay off the
  // instance (contextBridge copies own properties only).
  v8::Local<v8::Object> emitter = gin_helper::NewNodeEventEmitter(context);
  v8::Local<v8::Object> proto = v8::Object::New(isolate);
  proto->SetPrototype(context, emitter->GetPrototype()).Check();
  emitter->SetPrototype(context, proto).Check();
  auto method = [&](const char* name, v8::FunctionCallback callback,
                    int length) {
    v8::Local<v8::String> key = gin::StringToSymbol(isolate, name);
    v8::Local<v8::Function> fn =
        v8::Function::New(context, callback, data, length,
                          v8::ConstructorBehavior::kThrow)
            .ToLocalChecked();
    fn->SetName(key);
    proto->DefineOwnProperty(context, key, fn, v8::DontEnum).Check();
  };
  method("send", EmitterMethods<T>::Send, 1);
  method("sendSync", EmitterMethods<T>::SendSync, 1);
  method("invoke", EmitterMethods<T>::Invoke, 1);
  if (!internal) {
    method("sendToHost", EmitterMethods<T>::SendToHost, 1);
    method("postMessage", EmitterMethods<T>::PostMessage, 3);
  }
  return emitter;
}

template <typename T>
void CreateEmitters(v8::Local<v8::Context> context,
                    v8::Local<v8::Object> exports) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Value> transport;
  if (!gin::ConvertToV8(isolate, T::Create(isolate)).ToLocal(&transport))
    return;
  v8::Local<v8::Object> ipc_renderer =
      CreateEmitter<T>(context, transport, /*internal=*/false);
  v8::Local<v8::Object> ipc_renderer_internal =
      CreateEmitter<T>(context, transport, /*internal=*/true);
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("ipcRenderer", ipc_renderer);
  dict.Set("ipcRendererInternal", ipc_renderer_internal);

  // ipc_native::EmitIPCEvent() delivers incoming messages to these.
  gin_helper::Dictionary(isolate, context->Global())
      .SetHidden("ipcNative", exports);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  if (IsWorkerThread())
    CreateEmitters<IPCServiceWorker>(context, exports);
  else
    CreateEmitters<IPCRenderFrame>(context, exports);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_renderer_ipc, Initialize)
