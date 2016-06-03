// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/frame_subscriber.h"

#include "base/bind.h"
#include "atom/common/node_includes.h"
#include "content/public/browser/render_widget_host.h"

namespace atom {

namespace api {

FrameSubscriber::FrameSubscriber(v8::Isolate* isolate,
                                 content::RenderWidgetHostView* view,
                                 const FrameCaptureCallback& callback)
    : isolate_(isolate), view_(view), callback_(callback), weak_factory_(this) {
}

bool FrameSubscriber::ShouldCaptureFrame(
    const gfx::Rect& damage_rect,
    base::TimeTicks present_time,
    scoped_refptr<media::VideoFrame>* storage,
    DeliverFrameCallback* callback) {
  const auto host = view_ ? view_->GetRenderWidgetHost() : nullptr;
  if (!view_ || !host)
    return false;

  const auto size = view_->GetVisibleViewportSize();

  host->CopyFromBackingStore(
      gfx::Rect(size),
      size,
      base::Bind(&FrameSubscriber::OnFrameDelivered,
                 weak_factory_.GetWeakPtr(), callback_),
      kBGRA_8888_SkColorType);

  return false;
}

void FrameSubscriber::OnFrameDelivered(const FrameCaptureCallback& callback,
  const SkBitmap& bitmap, content::ReadbackResponse response) {
  if (bitmap.computeSize64() == 0)
    return;

  v8::Locker locker(isolate_);
  v8::HandleScope handle_scope(isolate_);

  size_t rgb_arr_size = bitmap.width() * bitmap.height() *
    bitmap.bytesPerPixel();
  v8::MaybeLocal<v8::Object> buffer = node::Buffer::New(isolate_, rgb_arr_size);
  if (buffer.IsEmpty())
    return;

  bitmap.copyPixelsTo(
    reinterpret_cast<uint8_t*>(node::Buffer::Data(buffer.ToLocalChecked())),
    rgb_arr_size);

  callback_.Run(buffer.ToLocalChecked());
}

}  // namespace api

}  // namespace atom
