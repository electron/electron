// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_autofill_driver_factory.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/electron_autofill_driver.h"

namespace electron {

AutofillDriverFactory::~AutofillDriverFactory() = default;

// static
void AutofillDriverFactory::BindAutofillDriver(
    mojo::PendingAssociatedReceiver<mojom::ElectronAutofillDriver>
        pending_receiver,
    content::RenderFrameHost* render_frame_host) {
  DCHECK(render_frame_host);

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  DCHECK(web_contents);

  AutofillDriverFactory* factory = FromWebContents(web_contents);
  if (!factory) {
    // The message pipe will be closed and raise a connection error to peer
    // side. The peer side can reconnect later when needed.
    return;
  }

  if (auto* driver = factory->DriverForFrame(render_frame_host))
    driver->BindPendingReceiver(std::move(pending_receiver));
}

AutofillDriverFactory::AutofillDriverFactory(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AutofillDriverFactory>(*web_contents) {}

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
  auto insertion_result = driver_map_.emplace(render_frame_host, nullptr);
  std::unique_ptr<AutofillDriver>& driver = insertion_result.first->second;
  bool insertion_happened = insertion_result.second;
  if (insertion_happened) {
    // The `render_frame_host` may already be deleted (or be in the process of
    // being deleted). In this case, we must not create a new driver. Otherwise,
    // a driver might hold a deallocated RFH.
    //
    // For example, `render_frame_host` is deleted in the following sequence:
    // 1. `render_frame_host->~RenderFrameHostImpl()` starts and marks
    //    `render_frame_host` as deleted.
    // 2. `ContentAutofillDriverFactory::RenderFrameDeleted(render_frame_host)`
    //    destroys the driver of `render_frame_host`.
    // 3. `SomeOtherWebContentsObserver::RenderFrameDeleted(render_frame_host)`
    //    calls `DriverForFrame(render_frame_host)`.
    // 5. `render_frame_host->~RenderFrameHostImpl()` finishes.
    if (render_frame_host->IsRenderFrameLive()) {
      driver = std::make_unique<AutofillDriver>(render_frame_host);
      DCHECK_EQ(driver_map_.find(render_frame_host)->second.get(),
                driver.get());
    } else {
      driver_map_.erase(insertion_result.first);
      DCHECK_EQ(driver_map_.count(render_frame_host), 0u);
      return nullptr;
    }
  }
  DCHECK(driver.get());
  return driver.get();
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

WEB_CONTENTS_USER_DATA_KEY_IMPL(AutofillDriverFactory);

}  // namespace electron
