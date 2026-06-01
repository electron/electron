// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_
#define ELECTRON_SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "shell/common/api/api.mojom.h"

namespace electron {

class RendererClientBase;

class ElectronApiServiceImpl
    : public mojom::ElectronRenderer,
      public mojom::ElectronFrameStartup,
      public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<ElectronApiServiceImpl> {
 public:
  ElectronApiServiceImpl(content::RenderFrame* render_frame,
                         RendererClientBase* renderer_client);
  ~ElectronApiServiceImpl() override;

  // disable copy
  ElectronApiServiceImpl(const ElectronApiServiceImpl&) = delete;
  ElectronApiServiceImpl& operator=(const ElectronApiServiceImpl&) = delete;

  void BindTo(mojo::PendingReceiver<mojom::ElectronRenderer> receiver);
  void BindFrameStartupReceiver(
      mojo::PendingAssociatedReceiver<mojom::ElectronFrameStartup> receiver);

  // mojom::ElectronRenderer
  void Message(bool internal,
               const std::string& channel,
               blink::CloneableMessage arguments) override;
  void ReceivePostMessage(const std::string& channel,
                          blink::TransferableMessage message) override;
  void TakeHeapSnapshot(mojo::ScopedHandle file,
                        TakeHeapSnapshotCallback callback) override;
  void ProcessPendingMessages();

  // mojom::ElectronFrameStartup
  void SetStartupData(mojom::RendererStartupDataPtr data) override;

  // The data pushed by the browser ahead of CommitNavigation, or null if it
  // has not arrived (the initial empty document of a fresh RenderFrame, or a
  // document that doesn't run the sandbox bundle). Consumers look preloads up
  // by id; a null here means there is nothing to run.
  const mojom::RendererStartupDataPtr& startup_data() const {
    return startup_data_;
  }

  // Stashes the startup data the browser attached to a CreateNewWindowReply
  // for an about-to-be-created window.open() child window. Picked up by the
  // next ElectronApiServiceImpl's constructor — the new RenderFrame is created
  // synchronously on the same call stack as RenderFrameImpl::CreateNewWindow().
  static void SetPendingNewWindowStartupData(
      mojom::RendererStartupDataPtr data);

  base::WeakPtr<ElectronApiServiceImpl> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  void OnInterfaceRequestForFrame(
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;

 private:
  // content::RenderFrameObserver
  void DidCreateDocumentElement() override;
  void OnDestruct() override;

  void OnConnectionError();

  // Whether the DOM document element has been created.
  bool document_created_ = false;
  service_manager::BinderRegistry registry_;

  mojo::PendingReceiver<mojom::ElectronRenderer> pending_receiver_;
  mojo::Receiver<mojom::ElectronRenderer> receiver_{this};
  mojo::AssociatedReceiver<mojom::ElectronFrameStartup> frame_startup_receiver_{
      this};

  // The most recent RendererStartupData pushed by the browser, consumed by the
  // sandboxed renderer client at DidCreateScriptContext time.
  mojom::RendererStartupDataPtr startup_data_;

  raw_ptr<RendererClientBase> renderer_client_;
  base::WeakPtrFactory<ElectronApiServiceImpl> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_
