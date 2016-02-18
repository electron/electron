// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/frame_subscriber.h"

#include "atom/common/node_includes.h"
#include "base/bind.h"
#include "media/base/video_frame.h"
#include "media/base/yuv_convert.h"
#include "content/public/browser/render_widget_host.h"
#include "ui/gfx/screen.h"

namespace atom {

namespace api {

using Subscriber = FrameSubscriber::Subscriber;

FrameSubscriber::FrameSubscriber(v8::Isolate* isolate,
                                 content::RenderWidgetHostView* view,
                                 const FrameCaptureCallback& callback)
    : isolate_(isolate), callback_(callback), pending_frames(0), view_(view) {
  subscriber_ = new Subscriber(this);
  size_ = view->GetVisibleViewportSize();
}

Subscriber::Subscriber(
  FrameSubscriber* frame_subscriber) : frame_subscriber_(frame_subscriber) {
}

Subscriber::~Subscriber() {
  frame_subscriber_->subscriber_ = NULL;
  frame_subscriber_->RequestDestruct();
}

bool Subscriber::ShouldCaptureFrame(
    const gfx::Rect& damage_rect,
    base::TimeTicks present_time,
    scoped_refptr<media::VideoFrame>* storage,
    DeliverFrameCallback* callback) {
  const auto view = frame_subscriber_->view_;
  const auto host = view ? view->GetRenderWidgetHost() : nullptr;
  if (!view || !host) {
    return false;
  }

  const gfx::Size view_size = view->GetViewBounds().size();

  gfx::Size bitmap_size = view_size;
  const gfx::NativeView native_view = view->GetNativeView();
  gfx::Screen* const screen = gfx::Screen::GetScreenFor(native_view);
  const float scale =
      screen->GetDisplayNearestWindow(native_view).device_scale_factor();
  if (scale > 1.0f)
    bitmap_size = gfx::ScaleToCeiledSize(view_size, scale);

  host->CopyFromBackingStore(
      gfx::Rect(view_size),
      bitmap_size,
      base::Bind(&FrameSubscriber::OnFrameDelivered,
                 base::Unretained(frame_subscriber_),
                 frame_subscriber_->callback_),
      kBGRA_8888_SkColorType);

  frame_subscriber_->pending_frames++;
  return false;
}

Subscriber* FrameSubscriber::GetSubscriber() {
  return subscriber_;
}

bool FrameSubscriber::RequestDestruct() {
  bool deletable = (subscriber_ == NULL && pending_frames == 0);
  // Destruct FrameSubscriber if we're not waiting for frames and the
  // subscription has ended
  if (deletable)
    delete this;

  return deletable;
}

void FrameSubscriber::OnFrameDelivered(const FrameCaptureCallback& callback,
  const SkBitmap& bitmap, content::ReadbackResponse response){
  pending_frames--;

  if (RequestDestruct() || subscriber_ == NULL || bitmap.computeSize64() == 0)
    return;

  v8::Locker locker(isolate_);
  v8::HandleScope handle_scope(isolate_);

  size_t rgb_arr_size = bitmap.width() * bitmap.height() *
    bitmap.bytesPerPixel();
  v8::MaybeLocal<v8::Object> buffer = node::Buffer::New(isolate_, rgb_arr_size);
  if (buffer.IsEmpty())
    return;

  bitmap.copyPixelsTo(
    reinterpret_cast<uint8*>(node::Buffer::Data(buffer.ToLocalChecked())),
    rgb_arr_size);

  callback_.Run(buffer.ToLocalChecked());
}

}  // namespace api

}  // namespace atom
