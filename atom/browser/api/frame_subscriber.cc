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

FrameSubscriber::FrameSubscriber(v8::Isolate* isolate,
                                 const gfx::Size& size,
                                 const FrameCaptureCallback& callback)
    : isolate_(isolate), size_(size), callback_(callback) {
}

bool FrameSubscriber::ShouldCaptureFrame(
    const gfx::Rect& damage_rect,
    base::TimeTicks present_time,
    scoped_refptr<media::VideoFrame>* storage,
    DeliverFrameCallback* callback) {
  *storage = media::VideoFrame::CreateFrame(media::VideoFrame::YV12, size_,
                                            gfx::Rect(size_), size_,
                                            base::TimeDelta());
  *callback = base::Bind(&FrameSubscriber::OnFrameDelivered,
                         base::Unretained(this),
                         *storage);
  return true;
}

void FrameSubscriber::OnFrameDelivered(
    scoped_refptr<media::VideoFrame> frame, base::TimeTicks, bool result) {
  if (!result)
    return;

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

  v8::Locker locker(isolate_);
  v8::HandleScope handle_scope(isolate_);
  callback_.Run(buffer.ToLocalChecked());
}

}  // namespace api

}  // namespace atom
