// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_autofill_driver_factory.h"

#include <memory>
#include <utility>
#include <vector>

#include "atom/browser/atom_autofill_driver.h"
#include "base/bind.h"
#include "base/callback.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace atom {

namespace {

std::unique_ptr<AutofillDriver> CreateDriver(
    content::RenderFrameHost* render_frame_host) {
  return std::make_unique<AutofillDriver>(render_frame_host);
}

}  // namespace

const char
    AutofillDriverFactory::kAtomAutofillDriverFactoryWebContentsUserDataKey[] =
        "atom_web_contents_autofill_driver_factory";

AutofillDriverFactory::~AutofillDriverFactory() {}

// static
void AutofillDriverFactory::CreateForWebContents(
    content::WebContents* contents) {
  if (FromWebContents(contents))
    return;

  auto new_factory = std::make_unique<AutofillDriverFactory>(contents);
  const std::vector<content::RenderFrameHost*> frames =
      contents->GetAllFrames();
  for (content::RenderFrameHost* frame : frames) {
    if (frame->IsRenderFrameLive())
      new_factory->RenderFrameCreated(frame);
  }

  contents->SetUserData(kAtomAutofillDriverFactoryWebContentsUserDataKey,
                        std::move(new_factory));
}

// static
AutofillDriverFactory* AutofillDriverFactory::FromWebContents(
    content::WebContents* contents) {
  return static_cast<AutofillDriverFactory*>(
      contents->GetUserData(kAtomAutofillDriverFactoryWebContentsUserDataKey));
}

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
  if (driver)
    driver->BindRequest(std::move(request));
}

AutofillDriverFactory::AutofillDriverFactory(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void AutofillDriverFactory::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  AddDriverForFrame(render_frame_host,
                    base::Bind(CreateDriver, render_frame_host));
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
    base::Callback<std::unique_ptr<AutofillDriver>()> factory_method) {
  auto insertion_result =
      driver_map_.insert(std::make_pair(render_frame_host, nullptr));
  // This can be called twice for the key representing the main frame.
  if (insertion_result.second) {
    insertion_result.first->second = factory_method.Run();
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

}  // namespace atom
