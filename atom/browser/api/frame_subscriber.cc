// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/frame_subscriber.h"

#include "base/bind.h"
#include "atom/common/node_includes.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "content/public/browser/render_widget_host.h"

#include <iostream>
#include "content/browser/compositor/image_transport_factory.h"
#include "cc/surfaces/surface_manager.h"
#include "cc/surfaces/surface.h"
#include "cc/output/copy_output_request.h"

namespace atom {

namespace api {

FrameSubscriberRenderWidgetHostView::FrameSubscriberRenderWidgetHostView() {
  std::cout << "FrameSubscriberRenderWidgetHostView" << std::endl;
}

FrameSubscriberRenderWidgetHostView::~FrameSubscriberRenderWidgetHostView() {
  std::cout << "~FrameSubscriberRenderWidgetHostView" << std::endl;
}

FrameSubscriber::FrameSubscriber(v8::Isolate* isolate,
                                 content::RenderWidgetHostView* view,
                                 const FrameCaptureCallback& callback,
                                 bool only_dirty)
    : isolate_(isolate), view_(view), callback_(callback),
      only_dirty_(only_dirty), weak_factory_(this) {
}

bool FrameSubscriber::ShouldCaptureFrame(
    const gfx::Rect& dirty_rect,
    base::TimeTicks present_time,
    scoped_refptr<media::VideoFrame>* storage,
    DeliverFrameCallback* callback) {
  const auto host = view_ ? view_->GetRenderWidgetHost() : nullptr;

  if (!view_ || !host)
    return false;

  /*if (dirty_rect.IsEmpty())
    return false;*/

  gfx::Rect rect = gfx::Rect(view_->GetVisibleViewportSize());
  if (only_dirty_)
    rect = dirty_rect;

  /*host->CopyFromBackingStore(
      rect,
      rect.size(),
      base::Bind(&FrameSubscriber::OnFrameDelivered,
                 weak_factory_.GetWeakPtr(), callback_, rect),
      kBGRA_8888_SkColorType);*/

  const auto base = reinterpret_cast<content::RenderWidgetHostViewBase*>(
    view_);
  content::ImageTransportFactory* itf =
    content::ImageTransportFactory::GetInstance();
  cc::SurfaceManager* sfm = itf->GetSurfaceManager();
  cc::Surface* surface = sfm->GetSurfaceForId(base->SurfaceIdForTesting());

  std::unique_ptr<cc::CopyOutputRequest> request =
    cc::CopyOutputRequest::CreateBitmapRequest(
      base::Bind(&FrameSubscriber::ReadbackResultAsBitmap,
      base::Unretained(this)));

  surface->RequestCopyOfOutput(std::move(request));
  return false;
}

void FrameSubscriber::ReadbackResultAsBitmap(
  std::unique_ptr<cc::CopyOutputResult> result) {
  std::unique_ptr<SkBitmap> bitmap = result->TakeBitmap();

  SkAutoPixmapUnlock res;
  if (!bitmap->requestLock(&res))
    return;

  std::cout << res.pixmap().addr() << std::endl;
}

void FrameSubscriber::OnFrameDelivered(const FrameCaptureCallback& callback,
  const gfx::Rect& damage_rect, const SkBitmap& bitmap,
  content::ReadbackResponse response) {
  if (response != content::ReadbackResponse::READBACK_SUCCESS)
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

  v8::Local<v8::Value> damage =
    mate::Converter<gfx::Rect>::ToV8(isolate_, damage_rect);

  callback_.Run(buffer.ToLocalChecked(), damage);
}

}  // namespace api

}  // namespace atom
