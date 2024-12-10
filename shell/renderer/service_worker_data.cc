// Copyright (c) 2024 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/renderer/service_worker_data.h"

#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/heap_snapshot.h"
#include "shell/renderer/electron_ipc_native.h"
#include "shell/renderer/preload_realm_context.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"

namespace electron {

ServiceWorkerData::~ServiceWorkerData() = default;

ServiceWorkerData::ServiceWorkerData(blink::WebServiceWorkerContextProxy* proxy,
                                     int64_t service_worker_version_id,
                                     const v8::Local<v8::Context>& v8_context)
    : proxy_(proxy),
      service_worker_version_id_(service_worker_version_id),
      isolate_(v8_context->GetIsolate()),
      v8_context_(v8_context->GetIsolate(), v8_context) {
  proxy_->GetAssociatedInterfaceRegistry()
      .AddInterface<mojom::ElectronRenderer>(
          base::BindRepeating(&ServiceWorkerData::OnElectronRendererRequest,
                              weak_ptr_factory_.GetWeakPtr()));
}

void ServiceWorkerData::OnElectronRendererRequest(
    mojo::PendingAssociatedReceiver<mojom::ElectronRenderer> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void ServiceWorkerData::Message(bool internal,
                                const std::string& channel,
                                blink::CloneableMessage arguments) {
  v8::Isolate* isolate = isolate_.get();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = v8_context_.Get(isolate_);

  v8::MaybeLocal<v8::Context> maybe_preload_context =
      preload_realm::GetPreloadRealmContext(context);

  if (maybe_preload_context.IsEmpty()) {
    return;
  }

  v8::Local<v8::Context> preload_context =
      maybe_preload_context.ToLocalChecked();
  v8::Context::Scope context_scope(preload_context);

  v8::Local<v8::Value> args = gin::ConvertToV8(isolate, arguments);

  ipc_native::EmitIPCEvent(preload_context, internal, channel, {}, args);
}

void ServiceWorkerData::ReceivePostMessage(const std::string& channel,
                                           blink::TransferableMessage message) {
  NOTIMPLEMENTED();
}

void ServiceWorkerData::TakeHeapSnapshot(mojo::ScopedHandle file,
                                         TakeHeapSnapshotCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

}  // namespace electron
