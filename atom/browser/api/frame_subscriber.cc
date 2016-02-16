// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/frame_subscriber.h"

#include "atom/common/node_includes.h"
#include "base/bind.h"
#include "media/base/video_frame.h"
#include "media/base/yuv_convert.h"

namespace atom {

namespace api {

using Subscriber = FrameSubscriber::Subscriber;

FrameSubscriber::FrameSubscriber(v8::Isolate* isolate,
                                 const gfx::Size& size,
                                 const FrameCaptureCallback& callback)
    : isolate_(isolate), size_(size), callback_(callback), pending_frames(0) {
  subscriber_ = new Subscriber(this);
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
  *storage = media::VideoFrame::CreateFrame(
      media::PIXEL_FORMAT_YV12,
      frame_subscriber_->size_, gfx::Rect(frame_subscriber_->size_),
      frame_subscriber_->size_, base::TimeDelta());
  *callback = base::Bind(&FrameSubscriber::OnFrameDelivered,
                         base::Unretained(frame_subscriber_), *storage);
  frame_subscriber_->pending_frames++;
  return true;
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

void FrameSubscriber::OnFrameDelivered(
    scoped_refptr<media::VideoFrame> frame, base::TimeTicks, bool result) {
  pending_frames--;

  if (RequestDestruct() || subscriber_ == NULL || !result)
    return;

  v8::Locker locker(isolate_);
  v8::HandleScope handle_scope(isolate_);

  gfx::Rect rect = frame->visible_rect();
  size_t rgb_arr_size = rect.width() * rect.height() * 4;
  v8::MaybeLocal<v8::Object> buffer = node::Buffer::New(isolate_, rgb_arr_size);
  if (buffer.IsEmpty())
    return;

  // Convert a frame of YUV to 32 bit ARGB.
  media::ConvertYUVToRGB32(frame->data(media::VideoFrame::kYPlane),
                           frame->data(media::VideoFrame::kUPlane),
                           frame->data(media::VideoFrame::kVPlane),
                           reinterpret_cast<uint8*>(
                              node::Buffer::Data(buffer.ToLocalChecked())),
                           rect.width(), rect.height(),
                           frame->stride(media::VideoFrame::kYPlane),
                           frame->stride(media::VideoFrame::kUVPlane),
                           rect.width() * 4,
                           media::YV12);

  callback_.Run(buffer.ToLocalChecked());
}

}  // namespace api

}  // namespace atom
