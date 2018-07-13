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
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/skbitmap_operations.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

FrameSubscriber::FrameSubscriber(v8::Isolate* isolate,
                                 content::WebContents* web_contents,
                                 const FrameCaptureCallback& callback,
                                 bool only_dirty)
    : content::WebContentsObserver(web_contents),
      isolate_(isolate),
      callback_(callback),
      only_dirty_(only_dirty),
      weak_ptr_factory_(this) {}

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
      base::BindOnce(&FrameSubscriber::Done, weak_ptr_factory_.GetWeakPtr(),
                     GetDamageRect()));
}

void FrameSubscriber::Done(const gfx::Rect& damage, const SkBitmap& frame) {
  if (frame.drawsNothing())
    return;

  v8::Locker locker(isolate_);
  v8::HandleScope handle_scope(isolate_);

  const_cast<SkBitmap&>(frame).setAlphaType(kPremul_SkAlphaType);
  const SkBitmap& bitmap = only_dirty_ ? SkBitmapOperations::CreateTiledBitmap(
                                             frame, damage.x(), damage.y(),
                                             damage.width(), damage.height())
                                       : frame;

  size_t rgb_row_size = bitmap.width() * bitmap.bytesPerPixel();
  auto source = static_cast<const char*>(bitmap.getPixels());

  v8::MaybeLocal<v8::Object> buffer =
      node::Buffer::Copy(isolate_, source, rgb_row_size * bitmap.height());
  auto local_buffer = buffer.ToLocalChecked();

  v8::Local<v8::Value> damage_rect =
      mate::Converter<gfx::Rect>::ToV8(isolate_, damage);

  callback_.Run(local_buffer, damage_rect);
}

}  // namespace api

}  // namespace atom
