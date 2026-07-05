// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_
#define ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_

#include <set>
#include <vector>

#include "base/functional/callback.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "third_party/blink/public/web/web_meaningful_layout.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-forward.h"

namespace electron {

class RendererClientBase;

// Helper class to forward the messages to the client.
class ElectronRenderFrameObserver
    : private content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<ElectronRenderFrameObserver> {
 public:
  using IsolatedWorldCreatedCallback = base::RepeatingCallback<void(int)>;

  ElectronRenderFrameObserver(content::RenderFrame* frame,
                              RendererClientBase* renderer_client);

  std::vector<int> GetIsolatedWorlds() const;
  void SetIsolatedWorldCreatedCallback(
      IsolatedWorldCreatedCallback isolated_world_created_callback);

  // disable copy
  ElectronRenderFrameObserver(const ElectronRenderFrameObserver&) = delete;
  ElectronRenderFrameObserver& operator=(const ElectronRenderFrameObserver&) =
      delete;

 private:
  ~ElectronRenderFrameObserver() override;

  // content::RenderFrameObserver:
  void DidClearWindowObject() override;
  void DidInstallConditionalFeatures(v8::Local<v8::Context> context,
                                     int world_id) override;
  void WillReleaseScriptContext(v8::Isolate* const isolate,
                                v8::Local<v8::Context> context,
                                int world_id) override;
  void OnDestruct() override;
  void DidMeaningfulLayout(blink::WebMeaningfulLayout layout_type) override;

  [[nodiscard]] bool ShouldNotifyClient(int world_id) const;

  void CreateIsolatedWorldContext();
  void EnsureConditionalFeaturesInstalled();

  // Blink may commit a document into the frame's existing V8 context instead
  // of creating a new one, so what has already been set up is tracked per
  // context, not per document. Both are cleared from
  // WillReleaseScriptContext().
  v8::Global<v8::Context> main_world_setup_context_;
  // The context DidCreateScriptContext() was called for: the isolated world
  // one with context isolation on, the main world one otherwise.
  v8::Global<v8::Context> client_notified_context_;
  std::set<int> isolated_worlds_;
  // Multiple JS wrappers can exist for the same frame, so fan out creation
  // notifications to each wrapper that subscribed during this document.
  std::vector<IsolatedWorldCreatedCallback> isolated_world_created_callbacks_;
  raw_ptr<content::RenderFrame> render_frame_;
  raw_ptr<RendererClientBase> renderer_client_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_
