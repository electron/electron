// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extensions_api_client.h"

#include <map>
#include <memory>
#include <string>

#include "extensions/browser/api/storage/settings_namespace.h"
#include "extensions/browser/guest_view/extensions_guest_view_manager_delegate.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest_delegate.h"
#include "printing/buildflags/buildflags.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/extensions/api/management/electron_management_api_delegate.h"
#include "shell/browser/extensions/api/storage/electron_sync_value_store_cache.h"
#include "shell/browser/extensions/electron_extension_web_contents_observer.h"
#include "shell/browser/extensions/electron_messaging_delegate.h"
#include "shell/common/gin_helper/handle.h"
#include "v8/include/v8.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/printing/browser/print_manager_utils.h"
#include "shell/browser/printing/print_view_manager_electron.h"
#endif

namespace extensions {

class ElectronGuestViewManagerDelegate
    : public ExtensionsGuestViewManagerDelegate {
 public:
  ElectronGuestViewManagerDelegate() = default;
  ~ElectronGuestViewManagerDelegate() override = default;

  // disable copy
  ElectronGuestViewManagerDelegate(const ElectronGuestViewManagerDelegate&) =
      delete;
  ElectronGuestViewManagerDelegate& operator=(
      const ElectronGuestViewManagerDelegate&) = delete;

  // GuestViewManagerDelegate:
  void OnGuestAdded(content::WebContents* guest_web_contents) const override {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);
    electron::api::WebContents::FromOrCreate(isolate, guest_web_contents);
  }
};

class ElectronMimeHandlerViewGuestDelegate
    : public MimeHandlerViewGuestDelegate {
 public:
  ElectronMimeHandlerViewGuestDelegate() = default;
  ~ElectronMimeHandlerViewGuestDelegate() override = default;

  // disable copy
  ElectronMimeHandlerViewGuestDelegate(
      const ElectronMimeHandlerViewGuestDelegate&) = delete;
  ElectronMimeHandlerViewGuestDelegate& operator=(
      const ElectronMimeHandlerViewGuestDelegate&) = delete;

  // MimeHandlerViewGuestDelegate.
  bool HandleContextMenu(content::RenderFrameHost& render_frame_host,
                         const content::ContextMenuParams& params) override {
    auto* web_contents =
        content::WebContents::FromRenderFrameHost(&render_frame_host);
    if (!web_contents)
      return true;

    electron::api::WebContents* api_web_contents =
        electron::api::WebContents::From(
            web_contents->GetOutermostWebContents());
    if (api_web_contents)
      api_web_contents->HandleContextMenu(render_frame_host, params);
    return true;
  }

  void RecordLoadMetric(bool in_main_frame,
                        const std::string& mime_type,
                        content::BrowserContext* browser_context) override {}
};

ElectronExtensionsAPIClient::ElectronExtensionsAPIClient() = default;
ElectronExtensionsAPIClient::~ElectronExtensionsAPIClient() = default;

void ElectronExtensionsAPIClient::AddAdditionalValueStoreCaches(
    content::BrowserContext* context,
    const scoped_refptr<value_store::ValueStoreFactory>& factory,
    SettingsChangedCallback observer,
    std::map<settings_namespace::Namespace,
             raw_ptr<ValueStoreCache, CtnExperimental>>* caches) {
  // Add support for chrome.storage.sync. There is no sync service in
  // Electron, so the cache is backed by a second local store, the same as
  // Chrome when sync is disabled. chrome.storage.managed stays unsupported
  // because there is no policy source to read from.
  (*caches)[settings_namespace::SYNC] =
      new ElectronSyncValueStoreCache(factory);
}

MessagingDelegate* ElectronExtensionsAPIClient::GetMessagingDelegate() {
  if (!messaging_delegate_)
    messaging_delegate_ = std::make_unique<ElectronMessagingDelegate>();
  return messaging_delegate_.get();
}

void ElectronExtensionsAPIClient::AttachWebContentsHelpers(
    content::WebContents* web_contents) const {
#if BUILDFLAG(ENABLE_PRINTING)
  electron::PrintViewManagerElectron::CreateForWebContents(web_contents);
  printing::CreateCompositeClientIfNeeded(web_contents, std::string());
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
ElectronExtensionsAPIClient::CreateGuestViewManagerDelegate() const {
  return std::make_unique<ElectronGuestViewManagerDelegate>();
}

}  // namespace extensions
