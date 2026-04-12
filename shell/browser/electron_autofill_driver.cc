// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_autofill_driver.h"

#include <memory>

#include <utility>

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "ui/gfx/geometry/rect_f.h"

namespace electron {

AutofillDriver::AutofillDriver(content::RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host) {
  autofill_popup_ = std::make_unique<AutofillPopup>();
}

AutofillDriver::~AutofillDriver() = default;

void AutofillDriver::BindPendingReceiver(
    mojo::PendingAssociatedReceiver<mojom::ElectronAutofillDriver>
        pending_receiver) {
  receiver_.Bind(std::move(pending_receiver));
}

void AutofillDriver::ShowAutofillPopup(
    const gfx::RectF& bounds,
    const std::vector<std::u16string>& values,
    const std::vector<std::u16string>& labels) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  auto* web_contents = api::WebContents::From(
      content::WebContents::FromRenderFrameHost(render_frame_host_));
  if (!web_contents)
    return;

  auto* owner_window = web_contents->owner_window();
  if (!owner_window)
    return;

  // |bounds| is supplied by the renderer in the calling frame's RenderWidget
  // coordinate space. Convert to the root view's space and clamp to the
  // calling frame's visible viewport so a (potentially compromised) subframe
  // cannot position the native popup over arbitrary application UI.
  auto* frame_view = render_frame_host_->GetView();
  if (!frame_view)
    return;
  gfx::RectF popup_bounds(bounds);
  popup_bounds.set_origin(
      frame_view->TransformPointToRootCoordSpaceF(popup_bounds.origin()));
  const gfx::Size frame_size = frame_view->GetViewBounds().size();
  const gfx::RectF frame_clip = gfx::BoundingRect(
      frame_view->TransformPointToRootCoordSpaceF(gfx::PointF()),
      frame_view->TransformPointToRootCoordSpaceF(
          gfx::PointF(frame_size.width(), frame_size.height())));
  popup_bounds.Intersect(frame_clip);
  if (popup_bounds.IsEmpty())
    return;

  auto* embedder = web_contents->embedder();

  bool osr =
      web_contents->IsOffScreen() || (embedder && embedder->IsOffScreen());
  content::RenderFrameHost* embedder_frame_host = nullptr;
  if (embedder) {
    auto* embedder_view =
        embedder->web_contents()->GetPrimaryMainFrame()->GetView();
    auto* view = web_contents->web_contents()->GetPrimaryMainFrame()->GetView();
    auto offset = view->GetViewBounds().origin() -
                  embedder_view->GetViewBounds().origin();
    popup_bounds.Offset(offset);
    embedder_frame_host = embedder->web_contents()->GetPrimaryMainFrame();
  }

  autofill_popup_->CreateView(render_frame_host_, embedder_frame_host, osr,
                              owner_window->content_view(), popup_bounds);
  autofill_popup_->SetItems(values, labels);
}

void AutofillDriver::HideAutofillPopup() {
  if (autofill_popup_)
    autofill_popup_->Hide();
}

}  // namespace electron
