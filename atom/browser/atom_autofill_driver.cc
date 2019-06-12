// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_autofill_driver.h"

#include <utility>

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/native_window.h"
#include "content/public/browser/render_widget_host_view.h"

namespace atom {

namespace {

const api::WebContents* WebContentsFromContentWebContents(
    content::WebContents* web_contents) {
  auto api_web_contents =
      api::WebContents::From(v8::Isolate::GetCurrent(), web_contents);
  if (!api_web_contents.IsEmpty()) {
    return api_web_contents.get();
  }

  return nullptr;
}

const api::WebContents* WebContentsFromRenderFrameHost(
    content::RenderFrameHost* render_frame_host) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);

  if (web_contents) {
    return WebContentsFromContentWebContents(web_contents);
  }

  return nullptr;
}

const api::WebContents* EmbedderFromWebContents(
    const api::WebContents* web_contents) {
  auto* embedder_web_contents = web_contents->HostWebContents();

  if (embedder_web_contents) {
    return WebContentsFromContentWebContents(embedder_web_contents);
  }

  return nullptr;
}

}  // namespace

AutofillDriver::AutofillDriver(content::RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host), binding_(this) {
  autofill_popup_.reset(new AutofillPopup());
}

AutofillDriver::~AutofillDriver() {}

void AutofillDriver::BindRequest(
    mojom::ElectronAutofillDriverAssociatedRequest request) {
  binding_.Bind(std::move(request));
}

void AutofillDriver::ShowAutofillPopup(
    const gfx::RectF& bounds,
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  auto* web_contents = WebContentsFromRenderFrameHost(render_frame_host_);
  if (!web_contents || !web_contents->owner_window())
    return;

  auto* embedder = EmbedderFromWebContents(web_contents);

  bool osr =
      web_contents->IsOffScreen() || (embedder && embedder->IsOffScreen());
  gfx::RectF popup_bounds(bounds);
  content::RenderFrameHost* embedder_frame_host = nullptr;
  if (embedder) {
    auto* embedder_view = embedder->web_contents()->GetMainFrame()->GetView();
    auto* view = web_contents->web_contents()->GetMainFrame()->GetView();
    auto offset = view->GetViewBounds().origin() -
                  embedder_view->GetViewBounds().origin();
    popup_bounds.Offset(offset.x(), offset.y());
    embedder_frame_host = embedder->web_contents()->GetMainFrame();
  }

  autofill_popup_->CreateView(render_frame_host_, embedder_frame_host, osr,
                              web_contents->owner_window()->content_view(),
                              bounds);
  autofill_popup_->SetItems(values, labels);
}

void AutofillDriver::HideAutofillPopup() {
  if (autofill_popup_)
    autofill_popup_->Hide();
}

}  // namespace atom
