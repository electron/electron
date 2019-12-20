// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extensions_api_client.h"

#include <memory>

#include "shell/browser/extensions/atom_extension_web_contents_observer.h"
#include "shell/browser/extensions/electron_messaging_delegate.h"

namespace extensions {

ElectronExtensionsAPIClient::ElectronExtensionsAPIClient() = default;
ElectronExtensionsAPIClient::~ElectronExtensionsAPIClient() = default;

MessagingDelegate* ElectronExtensionsAPIClient::GetMessagingDelegate() {
  if (!messaging_delegate_)
    messaging_delegate_ = std::make_unique<ElectronMessagingDelegate>();
  return messaging_delegate_.get();
}

void ElectronExtensionsAPIClient::AttachWebContentsHelpers(
    content::WebContents* web_contents) const {
  extensions::AtomExtensionWebContentsObserver::CreateForWebContents(
      web_contents);
}

}  // namespace extensions
