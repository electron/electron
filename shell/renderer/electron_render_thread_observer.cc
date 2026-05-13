// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/electron_render_thread_observer.h"

#include "base/command_line.h"
#include "base/no_destructor.h"
#include "base/threading/scoped_blocking_call.h"
#include "shell/common/options_switches.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"

namespace electron {

namespace {

// Process-global SW startup data. Written on the renderer main thread,
// read from worker threads. The WaitableEvent covers a dedicated SW process
// spinning up a worker before the main thread drains the push task.
struct WorkerStartupState {
  base::Lock lock;
  mojom::RendererStartupDataPtr data GUARDED_BY(lock);
  base::WaitableEvent ready{base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED};
};

WorkerStartupState& GetState() {
  static base::NoDestructor<WorkerStartupState> state;
  return *state;
}

}  // namespace

ElectronRenderThreadObserver::ElectronRenderThreadObserver() = default;
ElectronRenderThreadObserver::~ElectronRenderThreadObserver() = default;

// static
mojom::RendererStartupDataPtr
ElectronRenderThreadObserver::GetWorkerStartupData() {
  // No --service-worker-preload → no push and no SW realm. Bail rather than
  // wait forever.
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kServiceWorkerPreload)) {
    return nullptr;
  }
  auto& state = GetState();
  if (!state.ready.IsSignaled()) {
    // Worker thread raced the main thread's push task — wait for it.
    base::ScopedBlockingCall blocking(FROM_HERE, base::BlockingType::MAY_BLOCK);
    state.ready.Wait();
  }
  base::AutoLock auto_lock(state.lock);
  return state.data ? state.data.Clone() : nullptr;
}

void ElectronRenderThreadObserver::RegisterMojoInterfaces(
    blink::AssociatedInterfaceRegistry* associated_interfaces) {
  associated_interfaces->AddInterface<mojom::ElectronWorkerStartup>(
      base::BindRepeating(&ElectronRenderThreadObserver::OnWorkerStartupRequest,
                          base::Unretained(this)));
}

void ElectronRenderThreadObserver::UnregisterMojoInterfaces(
    blink::AssociatedInterfaceRegistry* associated_interfaces) {
  associated_interfaces->RemoveInterface(mojom::ElectronWorkerStartup::Name_);
}

void ElectronRenderThreadObserver::SetWorkerStartupData(
    mojom::RendererStartupDataPtr data) {
  auto& state = GetState();
  {
    base::AutoLock auto_lock(state.lock);
    state.data = std::move(data);
  }
  state.ready.Signal();
}

void ElectronRenderThreadObserver::OnWorkerStartupRequest(
    mojo::PendingAssociatedReceiver<mojom::ElectronWorkerStartup> receiver) {
  if (receiver_.is_bound())
    receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

}  // namespace electron
