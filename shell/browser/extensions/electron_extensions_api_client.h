// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_API_CLIENT_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_API_CLIENT_H_

#include <memory>

#include "extensions/browser/api/extensions_api_client.h"

namespace extensions {

class ElectronMessagingDelegate;

class ElectronExtensionsAPIClient : public ExtensionsAPIClient {
 public:
  ElectronExtensionsAPIClient();
  ~ElectronExtensionsAPIClient() override;

  // ExtensionsAPIClient
  MessagingDelegate* GetMessagingDelegate() override;
  void AttachWebContentsHelpers(
      content::WebContents* web_contents) const override;
  std::unique_ptr<MimeHandlerViewGuestDelegate>
  CreateMimeHandlerViewGuestDelegate(
      MimeHandlerViewGuest* guest) const override;
  ManagementAPIDelegate* CreateManagementAPIDelegate() const override;
  std::unique_ptr<guest_view::GuestViewManagerDelegate>
  CreateGuestViewManagerDelegate() const override;

 private:
  std::unique_ptr<ElectronMessagingDelegate> messaging_delegate_;
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_API_CLIENT_H_
