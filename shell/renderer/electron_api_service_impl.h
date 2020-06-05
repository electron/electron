// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_
#define SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "electron/buildflags/buildflags.h"
#include "electron/shell/common/api/api.mojom.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"

namespace electron {

class RendererClientBase;

class ElectronApiServiceImpl : public mojom::ElectronRenderer,
                               public content::RenderFrameObserver {
 public:
  ElectronApiServiceImpl(content::RenderFrame* render_frame,
                         RendererClientBase* renderer_client);

  void BindTo(
      mojo::PendingAssociatedReceiver<mojom::ElectronRenderer> receiver);

  void Message(bool internal,
               bool send_to_all,
               const std::string& channel,
               blink::CloneableMessage arguments,
               int32_t sender_id) override;
  void ReceivePostMessage(const std::string& channel,
                          blink::TransferableMessage message) override;
#if BUILDFLAG(ENABLE_REMOTE_MODULE)
  void DereferenceRemoteJSCallback(const std::string& context_id,
                                   int32_t object_id) override;
#endif
  void NotifyUserActivation() override;
  void TakeHeapSnapshot(mojo::ScopedHandle file,
                        TakeHeapSnapshotCallback callback) override;

  base::WeakPtr<ElectronApiServiceImpl> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  ~ElectronApiServiceImpl() override;

  // RenderFrameObserver implementation.
  void DidCreateDocumentElement() override;
  void OnDestruct() override;

  void OnConnectionError();

  // Whether the DOM document element has been created.
  bool document_created_ = false;

  mojo::AssociatedReceiver<mojom::ElectronRenderer> receiver_{this};

  RendererClientBase* renderer_client_;
  base::WeakPtrFactory<ElectronApiServiceImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ElectronApiServiceImpl);
};

}  // namespace electron

#endif  // SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_
