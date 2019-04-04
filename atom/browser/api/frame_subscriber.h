// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
#define ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_

#include <memory>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "components/viz/host/client_frame_sink_video_capturer.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "v8/include/v8.h"

namespace gfx {
class Image;
}

namespace atom {

namespace api {

class WebContents;

class FrameSubscriber : public content::WebContentsObserver,
                        public viz::mojom::FrameSinkVideoConsumer {
 public:
  using FrameCaptureCallback =
      base::Callback<void(const gfx::Image&, const gfx::Rect&)>;

  FrameSubscriber(content::WebContents* web_contents,
                  const FrameCaptureCallback& callback,
                  bool only_dirty);
  ~FrameSubscriber() override;

 private:
  void AttachToHost(content::RenderWidgetHost* host);
  void DetachFromHost();

  void RenderViewCreated(content::RenderViewHost* host) override;
  void RenderViewDeleted(content::RenderViewHost* host) override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;

  // viz::mojom::FrameSinkVideoConsumer implementation.
  void OnFrameCaptured(
      base::ReadOnlySharedMemoryRegion data,
      ::media::mojom::VideoFrameInfoPtr info,
      const gfx::Rect& update_rect,
      const gfx::Rect& content_rect,
      viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks) override;
  void OnStopped() override;

  void Done(const gfx::Rect& damage, const SkBitmap& frame);

  FrameCaptureCallback callback_;
  bool only_dirty_;

  content::RenderWidgetHost* host_;
  std::unique_ptr<viz::ClientFrameSinkVideoCapturer> video_capturer_;

  base::WeakPtrFactory<FrameSubscriber> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FrameSubscriber);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
