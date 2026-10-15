// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license.
#ifndef ELECTRON_SHELL_BROWSER_NATIVE_VIDEO_FRAME_REGISTRY_H_
#define ELECTRON_SHELL_BROWSER_NATIVE_VIDEO_FRAME_REGISTRY_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/document_user_data.h"
#include "mojo/public/cpp/bindings/associated_receiver_set.h"
#include "shell/common/native_video_frame.mojom.h"
#include "v8/include/v8.h"

namespace electron {

class NativeVideoFrameRegistry
    : public content::DocumentUserData<NativeVideoFrameRegistry>,
      public mojom::NativeVideoFrameHost {
 public:
  ~NativeVideoFrameRegistry() override;
  static void Bind(
      content::RenderFrameHost* frame,
      mojo::PendingAssociatedReceiver<mojom::NativeVideoFrameHost> receiver);
  static v8::Local<v8::Value> Resolve(v8::Isolate* isolate,
                                      content::RenderFrameHost* frame,
                                      const std::string& id);

 private:
  struct Entry;
  explicit NativeVideoFrameRegistry(content::RenderFrameHost* frame);
  void Export(gfx::GpuMemoryBufferHandle handle,
              mojom::NativeVideoFrameInfoPtr info,
              mojo::PendingRemote<mojom::NativeVideoFrameLease> lease,
              ExportCallback callback) override;
  void Release(const std::string& id, ReleaseCallback callback) override;
  void Expire(const std::string& id);

  std::map<std::string, std::shared_ptr<Entry>> entries_;
  mojo::AssociatedReceiverSet<mojom::NativeVideoFrameHost> receivers_;
  base::WeakPtrFactory<NativeVideoFrameRegistry> weak_factory_{this};
  friend content::DocumentUserData<NativeVideoFrameRegistry>;
  DOCUMENT_USER_DATA_KEY_DECL();
};

}  // namespace electron
#endif  // ELECTRON_SHELL_BROWSER_NATIVE_VIDEO_FRAME_REGISTRY_H_
