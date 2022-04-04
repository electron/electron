// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ELECTRON_BROWSER_HANDLER_IMPL_H_
#define SHELL_BROWSER_ELECTRON_BROWSER_HANDLER_IMPL_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "electron/shell/common/api/api.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "shell/browser/api/electron_api_web_contents.h"

namespace content {
class RenderFrameHost;
}

namespace electron {
class ElectronBrowserHandlerImpl : public mojom::ElectronBrowser,
                                   public content::WebContentsObserver {
 public:
  explicit ElectronBrowserHandlerImpl(
      content::RenderFrameHost* render_frame_host,
      mojo::PendingReceiver<mojom::ElectronBrowser> receiver);

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
  void MessageTo(int32_t web_contents_id,
                 const std::string& channel,
                 blink::CloneableMessage arguments) override;
  void MessageHost(const std::string& channel,
                   blink::CloneableMessage arguments) override;
  void UpdateDraggableRegions(
      std::vector<mojom::DraggableRegionPtr> regions) override;
  void SetTemporaryZoomLevel(double level) override;
  void DoGetZoomLevel(DoGetZoomLevelCallback callback) override;

  base::WeakPtr<ElectronBrowserHandlerImpl> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  ~ElectronBrowserHandlerImpl() override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

  void OnConnectionError();

  content::RenderFrameHost* GetRenderFrameHost();

  const int render_process_id_;
  const int render_frame_id_;

  mojo::Receiver<mojom::ElectronBrowser> receiver_{this};

  base::WeakPtrFactory<ElectronBrowserHandlerImpl> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ElectronBrowserHandlerImpl);
};
}  // namespace electron
#endif  // SHELL_BROWSER_ELECTRON_BROWSER_HANDLER_IMPL_H_
