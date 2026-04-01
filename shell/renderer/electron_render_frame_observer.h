// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_
#define ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_

#include <set>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "third_party/blink/public/web/web_local_frame.h"

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

  bool has_delayed_node_initialization_ = false;
  std::set<int> isolated_worlds_;
  // Multiple JS wrappers can exist for the same frame, so fan out creation
  // notifications to each wrapper that subscribed during this document.
  std::vector<IsolatedWorldCreatedCallback> isolated_world_created_callbacks_;
  raw_ptr<content::RenderFrame> render_frame_;
  raw_ptr<RendererClientBase> renderer_client_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_
