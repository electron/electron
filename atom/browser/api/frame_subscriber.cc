// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/frame_subscriber.h"

#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/node_includes.h"
#include "base/bind.h"
#include "content/public/browser/render_widget_host.h"
#include "ui/display/screen.h"
#include "ui/display/display.h"

namespace atom {

namespace api {

FrameSubscriber::FrameSubscriber(content::RenderWidgetHostView* view,
                                 const FrameCaptureCallback& callback,
                                 bool only_dirty)
    : view_(view),
      callback_(callback),
      only_dirty_(only_dirty),
      weak_factory_(this) {
}

bool FrameSubscriber::ShouldCaptureFrame(
    const gfx::Rect& dirty_rect,
    base::TimeTicks present_time,
    scoped_refptr<media::VideoFrame>* storage,
    DeliverFrameCallback* callback) {
  const auto host = view_ ? view_->GetRenderWidgetHost() : nullptr;
  if (!view_ || !host)
    return false;

  if (dirty_rect.IsEmpty())
    return false;

  gfx::Rect rect = gfx::Rect(view_->GetVisibleViewportSize());
  if (only_dirty_)
    rect = dirty_rect;

  gfx::Size view_size = rect.size();
  gfx::Size bitmap_size = view_size;
  const gfx::NativeView native_view = view_->GetNativeView();
  const float scale =
      display::Screen::GetScreen()->GetDisplayNearestWindow(native_view)
      .device_scale_factor();
  if (scale > 1.0f)
    bitmap_size = gfx::ScaleToCeiledSize(view_size, scale);

  rect = gfx::Rect(rect.origin(), bitmap_size);

  host->CopyFromBackingStore(
      rect,
      rect.size(),
      base::Bind(&FrameSubscriber::OnFrameDelivered,
                 weak_factory_.GetWeakPtr(), callback_, rect),
      kBGRA_8888_SkColorType);

  return false;
}

void FrameSubscriber::OnFrameDelivered(const FrameCaptureCallback& callback,
                                       const gfx::Rect& damage_rect,
                                       const SkBitmap& bitmap,
                                       content::ReadbackResponse response) {
  if (response != content::ReadbackResponse::READBACK_SUCCESS)
    return;

  callback.Run(gfx::Image::CreateFrom1xBitmap(bitmap), damage_rect);
}

}  // namespace api

}  // namespace atom
