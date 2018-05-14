// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/frame_subscriber.h"

#include "atom/common/native_mate_converters/gfx_converter.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/service/surfaces/surface_manager.h"
#include "content/browser/compositor/surface_utils.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"

namespace atom {

namespace api {

FrameSubscriber::FrameSubscriber(v8::Isolate* isolate,
                                 content::WebContents* web_contents,
                                 const FrameCaptureCallback& callback)
    : content::WebContentsObserver(web_contents),
      isolate_(isolate),
      callback_(callback) {}

FrameSubscriber::~FrameSubscriber() = default;

gfx::Rect FrameSubscriber::GetDamageRect() {
  auto* view = web_contents()->GetRenderWidgetHostView();
  if (view == nullptr)
    return gfx::Rect();

  auto* view_base = static_cast<content::RenderWidgetHostViewBase*>(view);
  viz::SurfaceManager* surface_manager =
      content::GetFrameSinkManager()->surface_manager();
  viz::Surface* surface =
      surface_manager->GetSurfaceForId(view_base->GetCurrentSurfaceId());
  if (surface == nullptr)
    return gfx::Rect();

  if (surface->HasActiveFrame()) {
    const viz::CompositorFrame& frame = surface->GetActiveFrame();
    viz::RenderPass* root_pass = frame.render_pass_list.back().get();
    gfx::Size frame_size = root_pass->output_rect.size();
    gfx::Rect damage_rect =
        gfx::ToEnclosingRect(gfx::RectF(root_pass->damage_rect));
    damage_rect.Intersect(gfx::Rect(frame_size));
    return gfx::ScaleToEnclosedRect(damage_rect,
                                    1.0f / frame.device_scale_factor());
  } else {
    return gfx::Rect();
  }
}

void FrameSubscriber::DidReceiveCompositorFrame() {
  auto* view = web_contents()->GetRenderWidgetHostView();
  if (view == nullptr)
    return;

  view->CopyFromSurface(
      gfx::Rect(), view->GetViewBounds().size(),
      base::BindOnce(&FrameSubscriber::Done, base::Unretained(this),
                     GetDamageRect()));
}

void FrameSubscriber::Done(const gfx::Rect& damage, const SkBitmap& frame) {
  if (frame.drawsNothing())
    return;

  const_cast<SkBitmap&>(frame).setAlphaType(kPremul_SkAlphaType);
  v8::Local<v8::Value> damage_rect =
      mate::Converter<gfx::Rect>::ToV8(isolate_, damage);
  callback_.Run(gfx::Image::CreateFrom1xBitmap(frame), damage_rect);
}

}  // namespace api

}  // namespace atom
