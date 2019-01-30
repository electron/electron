// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_render_widget_host_view.h"

#import <Cocoa/Cocoa.h>

#include "base/strings/utf_string_conversions.h"
#include "content/common/view_messages.h"
#include "ui/accelerated_widget_mac/accelerated_widget_mac.h"
#include "ui/display/screen.h"

#include "components/viz/common/features.h"

namespace atom {

class MacHelper : public content::BrowserCompositorMacClient,
                  public ui::AcceleratedWidgetMacNSView {
 public:
  explicit MacHelper(OffScreenRenderWidgetHostView* view) : view_(view) {
    [view_->GetNativeView().GetNativeNSView() setWantsLayer:YES];
  }

  virtual ~MacHelper() {}

  // content::BrowserCompositorMacClient:
  SkColor BrowserCompositorMacGetGutterColor() const override {
    // When making an element on the page fullscreen the element's background
    // may not match the page's, so use black as the gutter color to avoid
    // flashes of brighter colors during the transition.
    if (view_->render_widget_host()->delegate() &&
        view_->render_widget_host()->delegate()->IsFullscreenForCurrentTab()) {
      return SK_ColorBLACK;
    }
    return view_->last_frame_root_background_color();
  }

  void BrowserCompositorMacOnBeginFrame(base::TimeTicks frame_time) override {}

  void OnFrameTokenChanged(uint32_t frame_token) override {
    view_->render_widget_host()->DidProcessFrame(frame_token);
  }

  void AcceleratedWidgetCALayerParamsUpdated() override {}

  void DestroyCompositorForShutdown() override {}

  bool OnBrowserCompositorSurfaceIdChanged() override {
    return view_->render_widget_host()->SynchronizeVisualProperties();
  }

  std::vector<viz::SurfaceId> CollectSurfaceIdsForEviction() override {
    return view_->render_widget_host()->CollectSurfaceIdsForEviction();
  }

 private:
  OffScreenRenderWidgetHostView* view_;

  DISALLOW_COPY_AND_ASSIGN(MacHelper);
};

void OffScreenRenderWidgetHostView::SetActive(bool active) {}

void OffScreenRenderWidgetHostView::ShowDefinitionForSelection() {}

void OffScreenRenderWidgetHostView::SpeakSelection() {}

bool OffScreenRenderWidgetHostView::UpdateNSViewAndDisplay() {
  return browser_compositor_->UpdateSurfaceFromNSView(
      GetRootLayer()->bounds().size(), GetDisplay());
}

void OffScreenRenderWidgetHostView::CreatePlatformWidget(
    bool is_guest_view_hack) {
  mac_helper_ = new MacHelper(this);
  browser_compositor_.reset(new content::BrowserCompositorMac(
      mac_helper_, mac_helper_, render_widget_host_->is_hidden(), GetDisplay(),
      AllocateFrameSinkId(is_guest_view_hack)));

  if (!base::FeatureList::IsEnabled(features::kVizDisplayCompositor)) {
    SetNeedsBeginFrames(true);
  }
}

void OffScreenRenderWidgetHostView::DestroyPlatformWidget() {
  browser_compositor_.reset();
  delete mac_helper_;
}

viz::ScopedSurfaceIdAllocator
OffScreenRenderWidgetHostView::DidUpdateVisualProperties(
    const cc::RenderFrameMetadata& metadata) {
  base::OnceCallback<void()> allocation_task = base::BindOnce(
      base::IgnoreResult(
          &OffScreenRenderWidgetHostView::OnDidUpdateVisualPropertiesComplete),
      weak_ptr_factory_.GetWeakPtr(), metadata);
  return browser_compositor_->GetScopedRendererSurfaceIdAllocator(
      std::move(allocation_task));
}

display::Display OffScreenRenderWidgetHostView::GetDisplay() {
  content::ScreenInfo screen_info;
  GetScreenInfo(&screen_info);

  // Start with a reasonable display representation.
  display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestView(nullptr);

  // Populate attributes based on |screen_info|.
  display.set_bounds(screen_info.rect);
  display.set_work_area(screen_info.available_rect);
  display.set_device_scale_factor(screen_info.device_scale_factor);
  display.set_color_space(screen_info.color_space);
  display.set_color_depth(screen_info.depth);
  display.set_depth_per_component(screen_info.depth_per_component);
  display.set_is_monochrome(screen_info.is_monochrome);
  display.SetRotationAsDegree(screen_info.orientation_angle);

  return display;
}

void OffScreenRenderWidgetHostView::OnDidUpdateVisualPropertiesComplete(
    const cc::RenderFrameMetadata& metadata) {
  DCHECK_EQ(current_device_scale_factor_, metadata.device_scale_factor);
  browser_compositor_->UpdateSurfaceFromChild(
      metadata.device_scale_factor, metadata.viewport_size_in_pixels,
      metadata.local_surface_id_allocation.value_or(
          viz::LocalSurfaceIdAllocation()));
}

const viz::LocalSurfaceIdAllocation&
OffScreenRenderWidgetHostView::GetLocalSurfaceIdAllocation() const {
  return browser_compositor_->GetRendererLocalSurfaceIdAllocation();
}

ui::Compositor* OffScreenRenderWidgetHostView::GetCompositor() const {
  return browser_compositor_->GetCompositor();
}

ui::Layer* OffScreenRenderWidgetHostView::GetRootLayer() const {
  return browser_compositor_->GetRootLayer();
}

content::DelegatedFrameHost*
OffScreenRenderWidgetHostView::GetDelegatedFrameHost() const {
  return browser_compositor_->GetDelegatedFrameHost();
}

}  // namespace atom
