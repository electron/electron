// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_
#define SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_

#include <string>

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "electron/shell/common/api/api.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace electron {

class RendererClientBase;

class ElectronApiServiceImpl : public mojom::ElectronRenderer,
                               public content::RenderFrameObserver {
 public:
  static void CreateMojoService(
      content::RenderFrame* render_frame,
      RendererClientBase* renderer_client,
      mojom::ElectronRendererAssociatedRequest request);

  void Message(bool internal,
               bool send_to_all,
               const std::string& channel,
               base::Value arguments,
               int32_t sender_id) override;
  void UpdateCrashpadPipeName(const std::string& pipe_name) override;
  void TakeHeapSnapshot(mojo::ScopedHandle file,
                        TakeHeapSnapshotCallback callback) override;

 private:
  ~ElectronApiServiceImpl() override;
  ElectronApiServiceImpl(content::RenderFrame* render_frame,
                         RendererClientBase* renderer_client,
                         mojom::ElectronRendererAssociatedRequest request);

  // RenderFrameObserver implementation.
  void OnDestruct() override;

  mojo::AssociatedBinding<mojom::ElectronRenderer> binding_;

  content::RenderFrame* render_frame_;
  RendererClientBase* renderer_client_;

  DISALLOW_COPY_AND_ASSIGN(ElectronApiServiceImpl);
};

}  // namespace electron

#endif  // SHELL_RENDERER_ELECTRON_API_SERVICE_IMPL_H_
