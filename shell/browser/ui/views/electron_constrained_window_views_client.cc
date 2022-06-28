// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/electron_constrained_window_views_client.h"

#include "base/macros.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/platform_util.h"

namespace {

class ElectronConstrainedWindowViewsClient
    : public constrained_window::ConstrainedWindowViewsClient {
 public:
  ElectronConstrainedWindowViewsClient() {}
  ~ElectronConstrainedWindowViewsClient() override {}

  // disable copy
  ElectronConstrainedWindowViewsClient(
      const ElectronConstrainedWindowViewsClient&) = delete;
  ElectronConstrainedWindowViewsClient& operator=(
      const ElectronConstrainedWindowViewsClient&) = delete;

 private:
  // ConstrainedWindowViewsClient:
  web_modal::ModalDialogHost* GetModalDialogHost(
      gfx::NativeWindow parent) override {
    // Get the browser dialog management and hosting components from |parent|.
    auto windows = electron::WindowList::GetWindows();
    for (auto* window : windows) {
      if (window->GetNativeWindow() == parent)
        return window->GetWebContentsModalDialogHost();
    }

    return nullptr;
  }
  gfx::NativeView GetDialogHostView(gfx::NativeWindow parent) override {
    return platform_util::GetViewForWindow(parent);
  }
};

}  // namespace

std::unique_ptr<constrained_window::ConstrainedWindowViewsClient>
CreateElectronConstrainedWindowViewsClient() {
  return std::make_unique<ElectronConstrainedWindowViewsClient>();
}
