// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_WEB_CONTENTS_UTILITY_HANDLER_IMPL_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_WEB_CONTENTS_UTILITY_HANDLER_IMPL_H_

#include <string>
#include <vector>

#include "content/public/browser/render_frame_host_receiver_set.h"
#include "content/public/browser/web_contents_user_data.h"
#include "electron/shell/common/api/api.mojom.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "shell/browser/api/electron_api_web_contents.h"

namespace content {
class RenderFrameHost;
}

namespace electron {
class ElectronWebContentsUtilityHandlerImpl
    : public mojom::ElectronWebContentsUtility,
      public content::WebContentsUserData<
          ElectronWebContentsUtilityHandlerImpl> {
 public:
  static void BindElectronWebContentsUtility(
      mojo::PendingAssociatedReceiver<mojom::ElectronWebContentsUtility>
          receiver,
      content::RenderFrameHost* rfh);

  // disable copy
  ElectronWebContentsUtilityHandlerImpl(
      const ElectronWebContentsUtilityHandlerImpl&) = delete;
  ElectronWebContentsUtilityHandlerImpl& operator=(
      const ElectronWebContentsUtilityHandlerImpl&) = delete;

  ~ElectronWebContentsUtilityHandlerImpl() override;

  // mojom::ElectronWebContentsUtility:
  void OnFirstNonEmptyLayout() override;
  void UpdateDraggableRegions(
      std::vector<mojom::DraggableRegionPtr> regions) override;
  void SetTemporaryZoomLevel(double level) override;
  void DoGetZoomLevel(DoGetZoomLevelCallback callback) override;

 private:
  friend class content::WebContentsUserData<
      ElectronWebContentsUtilityHandlerImpl>;

  explicit ElectronWebContentsUtilityHandlerImpl(
      content::WebContents* web_contents);

  content::RenderFrameHostReceiverSet<mojom::ElectronWebContentsUtility>
      receivers_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};
}  // namespace electron
#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_WEB_CONTENTS_UTILITY_HANDLER_IMPL_H_
