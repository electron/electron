// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
#define ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_

#include "base/callback.h"
#include "content/public/browser/render_widget_host_view_frame_subscriber.h"
#include "ui/gfx/geometry/size.h"
#include "v8/include/v8.h"

namespace atom {

namespace api {

class FrameSubscriber : public content::RenderWidgetHostViewFrameSubscriber {
 public:
  using FrameCaptureCallback = base::Callback<void(v8::Local<v8::Value>)>;

  FrameSubscriber(v8::Isolate* isolate,
                  const gfx::Size& size,
                  const FrameCaptureCallback& callback);

  bool ShouldCaptureFrame(const gfx::Rect& damage_rect,
                          base::TimeTicks present_time,
                          scoped_refptr<media::VideoFrame>* storage,
                          DeliverFrameCallback* callback) override;

 private:
  void OnFrameDelivered(
      scoped_refptr<media::VideoFrame> frame, base::TimeTicks, bool);

  v8::Isolate* isolate_;
  gfx::Size size_;
  FrameCaptureCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(FrameSubscriber);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
