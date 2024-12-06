// Copyright (c) 2024 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_SERVICE_WORKER_DATA_H_
#define ELECTRON_SHELL_RENDERER_SERVICE_WORKER_DATA_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "electron/shell/common/api/api.mojom.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "third_party/blink/public/web/modules/service_worker/web_service_worker_context_proxy.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-forward.h"

namespace electron {

// Per ServiceWorker data in worker thread.
class ServiceWorkerData : public mojom::ElectronRenderer {
 public:
  ServiceWorkerData(blink::WebServiceWorkerContextProxy* proxy,
                    int64_t service_worker_version_id,
                    const v8::Local<v8::Context>& v8_context);
  ~ServiceWorkerData() override;

  // disable copy
  ServiceWorkerData(const ServiceWorkerData&) = delete;
  ServiceWorkerData& operator=(const ServiceWorkerData&) = delete;

  int64_t service_worker_version_id() const {
    return service_worker_version_id_;
  }

  blink::WebServiceWorkerContextProxy* proxy() const { return proxy_; }

  // mojom::ElectronRenderer
  void Message(bool internal,
               const std::string& channel,
               blink::CloneableMessage arguments) override;
  void ReceivePostMessage(const std::string& channel,
                          blink::TransferableMessage message) override;
  void TakeHeapSnapshot(mojo::ScopedHandle file,
                        TakeHeapSnapshotCallback callback) override;

 private:
  void OnElectronRendererRequest(
      mojo::PendingAssociatedReceiver<mojom::ElectronRenderer> receiver);

  raw_ptr<blink::WebServiceWorkerContextProxy> proxy_;
  const int64_t service_worker_version_id_;

  // The v8 context the bindings are accessible to.
  raw_ptr<v8::Isolate> isolate_;
  v8::Global<v8::Context> v8_context_;

  mojo::AssociatedReceiver<mojom::ElectronRenderer> receiver_{this};

  base::WeakPtrFactory<ServiceWorkerData> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_SERVICE_WORKER_DATA_H_
