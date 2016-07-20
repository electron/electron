// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_OSR_WEB_CONTENTS_VIEW_H_
#define ATOM_BROWSER_OSR_WEB_CONTENTS_VIEW_H_

// #include "content/browser/renderer_host/render_widget_host_view_base.h"
// #include "content/browser/renderer_host/delegated_frame_host.h"
// #include "content/browser/renderer_host/resize_lock.h"
// #include "third_party/WebKit/public/platform/WebVector.h"
// #include "cc/scheduler/begin_frame_source.h"
// #include "content/browser/renderer_host/render_widget_host_impl.h"
// #include "cc/output/compositor_frame.h"
// #include "ui/gfx/geometry/point.h"
// #include "base/threading/thread.h"
// #include "ui/compositor/compositor.h"
// #include "ui/compositor/layer_delegate.h"
// #include "ui/compositor/layer_owner.h"
// #include "ui/base/ime/text_input_client.h"

#include "content/browser/web_contents/web_contents_view.h"

namespace atom {

class OffScreenWebContentsView : public content::WebContentsView {
public:
  OffScreenWebContentsView();
  ~OffScreenWebContentsView();

  gfx::NativeView GetNativeView() const override;
  gfx::NativeView GetContentNativeView() const override;
  gfx::NativeWindow GetTopLevelNativeWindow() const override;

  void GetContainerBounds(gfx::Rect* out) const override;
  void SizeContents(const gfx::Size& size) override;
  void Focus() override;
  void SetInitialFocus() override;
  void StoreFocus() override;
  void RestoreFocus() override;
  content::DropData* GetDropData() const override;
  gfx::Rect GetViewBounds() const override;

  void CreateView(
      const gfx::Size& initial_size, gfx::NativeView context) override;

  content::RenderWidgetHostViewBase* CreateViewForWidget(
      content::RenderWidgetHost* render_widget_host,
      bool is_guest_view_hack) override;
  content::RenderWidgetHostViewBase* CreateViewForPopupWidget(
      content::RenderWidgetHost* render_widget_host) override;

  void SetPageTitle(const base::string16& title) override;
  void RenderViewCreated(content::RenderViewHost* host) override;
  void RenderViewSwappedIn(content::RenderViewHost* host) override;
  void SetOverscrollControllerEnabled(bool enabled) override;

#if defined(OS_MACOSX)
  void SetAllowOtherViews(bool allow) override;
  bool GetAllowOtherViews() const override;
  bool IsEventTracking() const override;
  void CloseTabAfterEventTracking() override;
#endif

private:
  content::RenderWidgetHostViewBase* view_;
};
}  // namespace atom

#endif  // ATOM_BROWSER_OSR_WEB_CONTENTS_VIEW_H_
