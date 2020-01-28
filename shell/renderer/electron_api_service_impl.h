// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_
#define SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "electron/shell/common/api/api.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace electron {

class RendererClientBase;

class ElectronApiServiceImpl : public mojom::ElectronRenderer,
                               public content::RenderFrameObserver {
 public:
  ElectronApiServiceImpl(content::RenderFrame* render_frame,
                         RendererClientBase* renderer_client);

  void BindTo(mojom::ElectronRendererAssociatedRequest request);

  void Message(bool internal,
               bool send_to_all,
               const std::string& channel,
               base::ListValue arguments,
               int32_t sender_id) override;
  void UpdateCrashpadPipeName(const std::string& pipe_name) override;
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

  mojo::AssociatedBinding<mojom::ElectronRenderer> binding_;

  RendererClientBase* renderer_client_;
  base::WeakPtrFactory<ElectronApiServiceImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ElectronApiServiceImpl);
};

}  // namespace electron

#endif  // SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_
