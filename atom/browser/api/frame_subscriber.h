// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
#define ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_

#include "base/callback.h"
#include "v8/include/v8.h"

#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/render_widget_host_view_frame_subscriber.h"
#include "content/public/browser/readback_types.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/size.h"

namespace atom {

namespace api {

class FrameSubscriber {
 public:
  using FrameCaptureCallback = base::Callback<void(v8::Local<v8::Value>)>;

  // Inner class that is the actual subscriber sent to chromium
  class Subscriber :
    public content::RenderWidgetHostViewFrameSubscriber {
   public:
    explicit Subscriber(FrameSubscriber* frame_subscriber);

    bool ShouldCaptureFrame(const gfx::Rect& damage_rect,
                            base::TimeTicks present_time,
                            scoped_refptr<media::VideoFrame>* storage,
                            DeliverFrameCallback* callback) override;

    ~Subscriber();
   private:
    FrameSubscriber* frame_subscriber_;
  };

  FrameSubscriber(v8::Isolate* isolate,
                  content::RenderWidgetHostView* view,
                  const FrameCaptureCallback& callback);

  Subscriber* GetSubscriber();

 private:
  void OnFrameDelivered(const FrameCaptureCallback& callback,
    const SkBitmap& bitmap, content::ReadbackResponse response);

  bool RequestDestruct();

  v8::Isolate* isolate_;
  content::RenderWidgetHostView* view_;
  FrameCaptureCallback callback_;
  Subscriber* subscriber_;
  int pending_frames;

  DISALLOW_COPY_AND_ASSIGN(FrameSubscriber);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
