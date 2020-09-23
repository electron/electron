// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extensions_api_client.h"

#include <memory>
#include <string>

#include "electron/buildflags/buildflags.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest_delegate.h"
#include "printing/buildflags/buildflags.h"
#include "shell/browser/extensions/api/management/electron_management_api_delegate.h"
#include "shell/browser/extensions/electron_extension_web_contents_observer.h"
#include "shell/browser/extensions/electron_messaging_delegate.h"

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "components/pdf/browser/pdf_web_contents_helper.h"  // nogncheck
#include "shell/browser/electron_pdf_web_contents_helper_client.h"
#endif

namespace extensions {

class ElectronMimeHandlerViewGuestDelegate
    : public MimeHandlerViewGuestDelegate {
 public:
  ElectronMimeHandlerViewGuestDelegate() {}
  ~ElectronMimeHandlerViewGuestDelegate() override {}

  // MimeHandlerViewGuestDelegate.
  bool HandleContextMenu(content::WebContents* web_contents,
                         const content::ContextMenuParams& params) override {
    // TODO(nornagon): surface this event to JS
    LOG(INFO) << "HCM";
    return true;
  }
  void RecordLoadMetric(bool in_main_frame,
                        const std::string& mime_type) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronMimeHandlerViewGuestDelegate);
};

ElectronExtensionsAPIClient::ElectronExtensionsAPIClient() = default;
ElectronExtensionsAPIClient::~ElectronExtensionsAPIClient() = default;

MessagingDelegate* ElectronExtensionsAPIClient::GetMessagingDelegate() {
  if (!messaging_delegate_)
    messaging_delegate_ = std::make_unique<ElectronMessagingDelegate>();
  return messaging_delegate_.get();
}

void ElectronExtensionsAPIClient::AttachWebContentsHelpers(
    content::WebContents* web_contents) const {
  extensions::ElectronExtensionWebContentsObserver::CreateForWebContents(
      web_contents);

#if BUILDFLAG(ENABLE_PDF_VIEWER)
  pdf::PDFWebContentsHelper::CreateForWebContentsWithClient(
      web_contents, std::make_unique<ElectronPDFWebContentsHelperClient>());
#endif
}

ManagementAPIDelegate*
ElectronExtensionsAPIClient::CreateManagementAPIDelegate() const {
  return new ElectronManagementAPIDelegate;
}

std::unique_ptr<MimeHandlerViewGuestDelegate>
ElectronExtensionsAPIClient::CreateMimeHandlerViewGuestDelegate(
    MimeHandlerViewGuest* guest) const {
  return std::make_unique<ElectronMimeHandlerViewGuestDelegate>();
}

}  // namespace extensions
