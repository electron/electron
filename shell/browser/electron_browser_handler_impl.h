// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ELECTRON_BROWSER_HANDLER_IMPL_H_
#define SHELL_BROWSER_ELECTRON_BROWSER_HANDLER_IMPL_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "electron/shell/common/api/api.mojom.h"
#include "shell/browser/api/electron_api_web_contents.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace electron {
class ElectronBrowserHandlerImpl : public mojom::ElectronBrowser {
 public:
  ~ElectronBrowserHandlerImpl() override;

  static void Create(content::RenderFrameHost* frame_host,
                     mojo::PendingReceiver<mojom::ElectronBrowser> receiver);

  // mojom::ElectronBrowser:
  void Message(bool internal,
               const std::string& channel,
               blink::CloneableMessage arguments) override;
  void Invoke(bool internal,
              const std::string& channel,
              blink::CloneableMessage arguments,
              InvokeCallback callback) override;
  void OnFirstNonEmptyLayout() override;
  void ReceivePostMessage(const std::string& channel,
                          blink::TransferableMessage message) override;
  void MessageSync(bool internal,
                   const std::string& channel,
                   blink::CloneableMessage arguments,
                   MessageSyncCallback callback) override;
  void MessageTo(bool internal,
                 bool send_to_all,
                 int32_t web_contents_id,
                 const std::string& channel,
                 blink::CloneableMessage arguments) override;
  void MessageHost(const std::string& channel,
                   blink::CloneableMessage arguments) override;
  void UpdateDraggableRegions(
      std::vector<mojom::DraggableRegionPtr> regions) override;
  void SetTemporaryZoomLevel(double level) override;
  void DoGetZoomLevel(DoGetZoomLevelCallback callback) override;

 private:
  explicit ElectronBrowserHandlerImpl(content::RenderFrameHost*);

  api::WebContents* GetAPIWebContents();

  const int render_process_id_;
  const int render_frame_id_;

  DISALLOW_COPY_AND_ASSIGN(ElectronBrowserHandlerImpl);
};
}  // namespace electron
#endif  // SHELL_BROWSER_ELECTRON_BROWSER_HANDLER_IMPL_H_
