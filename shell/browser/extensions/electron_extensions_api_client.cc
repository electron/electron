// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extensions_api_client.h"

#include <memory>
#include <string>

#include "electron/buildflags/buildflags.h"
#include "extensions/browser/guest_view/extensions_guest_view_manager_delegate.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest_delegate.h"
#include "printing/buildflags/buildflags.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/extensions/api/management/electron_management_api_delegate.h"
#include "shell/browser/extensions/electron_extension_web_contents_observer.h"
#include "shell/browser/extensions/electron_messaging_delegate.h"
#include "v8/include/v8.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/printing/browser/print_manager_utils.h"
#include "shell/browser/printing/print_preview_message_handler.h"
#include "shell/browser/printing/print_view_manager_electron.h"
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "components/pdf/browser/pdf_web_contents_helper.h"  // nogncheck
#include "shell/browser/electron_pdf_web_contents_helper_client.h"
#endif

namespace extensions {

class ElectronGuestViewManagerDelegate
    : public ExtensionsGuestViewManagerDelegate {
 public:
  explicit ElectronGuestViewManagerDelegate(content::BrowserContext* context)
      : ExtensionsGuestViewManagerDelegate(context) {}
  ~ElectronGuestViewManagerDelegate() override = default;

  // GuestViewManagerDelegate:
  void OnGuestAdded(content::WebContents* guest_web_contents) const override {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);
    electron::api::WebContents::FromOrCreate(isolate, guest_web_contents);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronGuestViewManagerDelegate);
};

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
#if BUILDFLAG(ENABLE_PRINTING)
  electron::PrintPreviewMessageHandler::CreateForWebContents(web_contents);
  electron::PrintViewManagerElectron::CreateForWebContents(web_contents);
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
  pdf::PDFWebContentsHelper::CreateForWebContentsWithClient(
      web_contents, std::make_unique<ElectronPDFWebContentsHelperClient>());
#endif

  extensions::ElectronExtensionWebContentsObserver::CreateForWebContents(
      web_contents);
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

std::unique_ptr<guest_view::GuestViewManagerDelegate>
ElectronExtensionsAPIClient::CreateGuestViewManagerDelegate(
    content::BrowserContext* context) const {
  return std::make_unique<ElectronGuestViewManagerDelegate>(context);
}

}  // namespace extensions
