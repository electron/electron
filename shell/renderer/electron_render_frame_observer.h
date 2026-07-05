// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_
#define ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

class RendererClientBase;

// Helper class to forward the messages to the client.
class ElectronRenderFrameObserver : private content::RenderFrameObserver {
 public:
  ElectronRenderFrameObserver(content::RenderFrame* frame,
                              RendererClientBase* renderer_client);
  ~ElectronRenderFrameObserver() override;

  // disable copy
  ElectronRenderFrameObserver(const ElectronRenderFrameObserver&) = delete;
  ElectronRenderFrameObserver& operator=(const ElectronRenderFrameObserver&) =
      delete;

 private:
  // content::RenderFrameObserver:
  void DidClearWindowObject() override;
  void DidInstallConditionalFeatures(v8::Local<v8::Context> context,
                                     int world_id) override;
  void WillReleaseScriptContext(v8::Isolate* const isolate,
                                v8::Local<v8::Context> context,
                                int world_id) override;
  void OnDestruct() override;
  void DidMeaningfulLayout(blink::WebMeaningfulLayout layout_type) override;
  void DidCommitProvisionalLoad(ui::PageTransition transition) override;

  [[nodiscard]] bool ShouldNotifyClient(int world_id) const;

  void CreateIsolatedWorldContext();
  void EnsureConditionalFeaturesInstalled();

  bool has_delayed_node_initialization_ = false;
  bool main_world_features_installed_ = false;
  bool isolated_world_features_installed_ = false;
  raw_ptr<content::RenderFrame> render_frame_;
  raw_ptr<RendererClientBase> renderer_client_;

  base::WeakPtrFactory<ElectronRenderFrameObserver> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_
