// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_autofill_driver_factory.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/atom_autofill_driver.h"

namespace electron {

namespace {

std::unique_ptr<AutofillDriver> CreateDriver(
    content::RenderFrameHost* render_frame_host,
    mojom::ElectronAutofillDriverAssociatedRequest request) {
  return std::make_unique<AutofillDriver>(render_frame_host,
                                          std::move(request));
}

}  // namespace

AutofillDriverFactory::~AutofillDriverFactory() = default;

// static
void AutofillDriverFactory::BindAutofillDriver(
    mojom::ElectronAutofillDriverAssociatedRequest request,
    content::RenderFrameHost* render_frame_host) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents)
    return;

  AutofillDriverFactory* factory =
      AutofillDriverFactory::FromWebContents(web_contents);
  if (!factory)
    return;

  AutofillDriver* driver = factory->DriverForFrame(render_frame_host);
  if (!driver)
    factory->AddDriverForFrame(
        render_frame_host,
        base::BindOnce(CreateDriver, render_frame_host, std::move(request)));
}

AutofillDriverFactory::AutofillDriverFactory(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  const std::vector<content::RenderFrameHost*> frames =
      web_contents->GetAllFrames();
  for (content::RenderFrameHost* frame : frames) {
    if (frame->IsRenderFrameLive())
      RenderFrameCreated(frame);
  }
}

void AutofillDriverFactory::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  DeleteDriverForFrame(render_frame_host);
}

void AutofillDriverFactory::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // For the purposes of this code, a navigation is not important if it has not
  // committed yet or if it's in a subframe.
  if (!navigation_handle->HasCommitted() ||
      !navigation_handle->IsInMainFrame()) {
    return;
  }

  CloseAllPopups();
}

AutofillDriver* AutofillDriverFactory::DriverForFrame(
    content::RenderFrameHost* render_frame_host) {
  auto mapping = driver_map_.find(render_frame_host);
  return mapping == driver_map_.end() ? nullptr : mapping->second.get();
}

void AutofillDriverFactory::AddDriverForFrame(
    content::RenderFrameHost* render_frame_host,
    CreationCallback factory_method) {
  auto insertion_result =
      driver_map_.insert(std::make_pair(render_frame_host, nullptr));
  // This can be called twice for the key representing the main frame.
  if (insertion_result.second) {
    insertion_result.first->second = std::move(factory_method).Run();
  }
}

void AutofillDriverFactory::DeleteDriverForFrame(
    content::RenderFrameHost* render_frame_host) {
  driver_map_.erase(render_frame_host);
}

void AutofillDriverFactory::CloseAllPopups() {
  for (auto& it : driver_map_) {
    it.second->HideAutofillPopup();
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AutofillDriverFactory)

}  // namespace electron
