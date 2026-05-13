// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_THREAD_OBSERVER_H_
#define ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_THREAD_OBSERVER_H_

#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "content/public/renderer/render_thread_observer.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "shell/common/api/api.mojom.h"

namespace blink {
class AssociatedInterfaceRegistry;
}

namespace electron {

// Per-process renderer observer. Receives the service-worker preload set +
// process info pushed by the browser at RenderProcessReady(), and makes it
// readable from any thread — specifically the service-worker worker thread
// where the preload realm is created (no RenderFrame, no per-frame mojo
// channel, no sync IPC).
class ElectronRenderThreadObserver : public content::RenderThreadObserver,
                                     public mojom::ElectronWorkerStartup {
 public:
  ElectronRenderThreadObserver();
  ~ElectronRenderThreadObserver() override;

  ElectronRenderThreadObserver(const ElectronRenderThreadObserver&) = delete;
  ElectronRenderThreadObserver& operator=(const ElectronRenderThreadObserver&) =
      delete;

  // Returns a copy of the worker startup data the browser has pushed, blocking
  // until it arrives if it has not yet. Safe to call from any thread.
  //
  // The block exists for one timing edge: a dedicated service-worker process
  // whose worker starts before the main thread has drained the per-process
  // SetWorkerStartupData() task. In practice the push lands at
  // RenderProcessReady() and the worker starts much later, so the wait is a
  // no-op. Returns null if the renderer was launched without a service-worker
  // preload registered (the browser never pushes in that case and the SW
  // realm is not created either).
  static mojom::RendererStartupDataPtr GetWorkerStartupData();

  // content::RenderThreadObserver:
  void RegisterMojoInterfaces(
      blink::AssociatedInterfaceRegistry* associated_interfaces) override;
  void UnregisterMojoInterfaces(
      blink::AssociatedInterfaceRegistry* associated_interfaces) override;

  // mojom::ElectronWorkerStartup:
  void SetWorkerStartupData(mojom::RendererStartupDataPtr data) override;

 private:
  void OnWorkerStartupRequest(
      mojo::PendingAssociatedReceiver<mojom::ElectronWorkerStartup> receiver);

  mojo::AssociatedReceiver<mojom::ElectronWorkerStartup> receiver_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_THREAD_OBSERVER_H_
