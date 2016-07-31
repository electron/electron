// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
#define ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/render_widget_host_view_frame_subscriber.h"
#include "content/public/browser/readback_types.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/size.h"
#include "v8/include/v8.h"

#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "cc/surfaces/surface_id.h"
#include "cc/output/copy_output_result.h"

namespace atom {

namespace api {

class FrameSubscriber : public content::RenderWidgetHostViewFrameSubscriber {
 public:
  using FrameCaptureCallback =
      base::Callback<void(v8::Local<v8::Value>, v8::Local<v8::Value>)>;

  FrameSubscriber(v8::Isolate* isolate,
                  content::RenderWidgetHostView* view,
                  const FrameCaptureCallback& callback,
                  bool only_dirty);

  bool ShouldCaptureFrame(const gfx::Rect& damage_rect,
                          base::TimeTicks present_time,
                          scoped_refptr<media::VideoFrame>* storage,
                          DeliverFrameCallback* callback) override;

 private:
  void ReadbackResultAsBitmap(
    std::unique_ptr<cc::CopyOutputResult> result);

  void OnFrameDelivered(const FrameCaptureCallback& callback,
                        const gfx::Rect& damage_rect,
                        const SkBitmap& bitmap,
                        content::ReadbackResponse response);

  v8::Isolate* isolate_;
  content::RenderWidgetHostView* view_;
  FrameCaptureCallback callback_;
  bool only_dirty_;

  base::WeakPtrFactory<FrameSubscriber> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FrameSubscriber);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
