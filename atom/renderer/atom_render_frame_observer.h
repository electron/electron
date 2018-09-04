// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_RENDER_FRAME_OBSERVER_H_
#define ATOM_RENDERER_ATOM_RENDER_FRAME_OBSERVER_H_

#include <string>

#include "atom/renderer/renderer_client_base.h"
#include "base/strings/string16.h"
#include "content/public/renderer/render_frame_observer.h"
#include "ipc/ipc_platform_file.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace base {
class ListValue;
}

namespace atom {

enum World {
  MAIN_WORLD = 0,
  // Use a high number far away from 0 to not collide with any other world
  // IDs created internally by Chrome.
  ISOLATED_WORLD = 999
};

// Helper class to forward the messages to the client.
class AtomRenderFrameObserver : public content::RenderFrameObserver {
 public:
  AtomRenderFrameObserver(content::RenderFrame* frame,
                          RendererClientBase* renderer_client);

  // content::RenderFrameObserver:
  void DidClearWindowObject() override;
  void DidCreateScriptContext(v8::Handle<v8::Context> context,
                              int world_id) override;
  void DraggableRegionsChanged() override;
  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                int world_id) override;
  void OnDestruct() override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void DidCreateDocumentElement() override;

 protected:
  virtual void EmitIPCEvent(blink::WebLocalFrame* frame,
                            const std::string& channel,
                            const base::ListValue& args,
                            int32_t sender_id);

 private:
  bool ShouldNotifyClient(int world_id);
  void CreateIsolatedWorldContext();
  bool IsMainWorld(int world_id);
  bool IsIsolatedWorld(int world_id);
  void OnBrowserMessage(bool send_to_all,
                        const std::string& channel,
                        const base::ListValue& args,
                        int32_t sender_id);
  void OnTakeHeapSnapshot(IPC::PlatformFileForTransit file_handle,
                          const std::string& channel);

  content::RenderFrame* render_frame_;
  RendererClientBase* renderer_client_;
  bool document_created_ = false;

  DISALLOW_COPY_AND_ASSIGN(AtomRenderFrameObserver);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_RENDER_FRAME_OBSERVER_H_
