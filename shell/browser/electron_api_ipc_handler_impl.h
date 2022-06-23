// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_API_IPC_HANDLER_IMPL_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_API_IPC_HANDLER_IMPL_H_

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
class ElectronApiIPCHandlerImpl
    : public mojom::ElectronApiIPC,
      public content::WebContentsUserData<ElectronApiIPCHandlerImpl> {
 public:
  static void BindElectronApiIPC(
      mojo::PendingAssociatedReceiver<mojom::ElectronApiIPC> receiver,
      content::RenderFrameHost* rfh);

  // disable copy
  ElectronApiIPCHandlerImpl(const ElectronApiIPCHandlerImpl&) = delete;
  ElectronApiIPCHandlerImpl& operator=(const ElectronApiIPCHandlerImpl&) =
      delete;

  ~ElectronApiIPCHandlerImpl() override;

  // mojom::ElectronApiIPC:
  void Message(bool internal,
               const std::string& channel,
               blink::CloneableMessage arguments) override;
  void Invoke(bool internal,
              const std::string& channel,
              blink::CloneableMessage arguments,
              InvokeCallback callback) override;
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

 private:
  friend class content::WebContentsUserData<ElectronApiIPCHandlerImpl>;

  explicit ElectronApiIPCHandlerImpl(content::WebContents* web_contents);

  content::RenderFrameHostReceiverSet<mojom::ElectronApiIPC> receivers_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};
}  // namespace electron
#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_API_IPC_HANDLER_IMPL_H_
