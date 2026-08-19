// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_web_contents_utility_handler_impl.h"

#include <utility>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/permission_descriptor_util.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "shell/browser/preload_code_cache.h"
#include "shell/browser/web_contents_permission_helper.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom.h"

namespace electron {

ElectronWebContentsUtilityHandlerImpl::ElectronWebContentsUtilityHandlerImpl(
    content::RenderFrameHost* frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronWebContentsUtility> receiver)
    : render_frame_host_token_(frame_host->GetGlobalFrameToken()) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame_host);
  DCHECK(web_contents);
  content::WebContentsObserver::Observe(web_contents);

  receiver_.Bind(std::move(receiver));
  receiver_.set_disconnect_handler(base::BindOnce(
      &ElectronWebContentsUtilityHandlerImpl::OnConnectionError, GetWeakPtr()));
}

ElectronWebContentsUtilityHandlerImpl::
    ~ElectronWebContentsUtilityHandlerImpl() = default;

void ElectronWebContentsUtilityHandlerImpl::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host->GetGlobalFrameToken() == render_frame_host_token_) {
    delete this;
  }
}

void ElectronWebContentsUtilityHandlerImpl::OnConnectionError() {
  delete this;
}

void ElectronWebContentsUtilityHandlerImpl::OnFirstNonEmptyLayout() {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->OnFirstNonEmptyLayout(GetRenderFrameHost());
  }
}

void ElectronWebContentsUtilityHandlerImpl::SetTemporaryZoomLevel(
    double level) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->SetTemporaryZoomLevel(level);
  }
}

void ElectronWebContentsUtilityHandlerImpl::SetPreloadCodeCache(
    const std::string& id,
    const std::vector<uint8_t>& source_hash,
    mojo_base::BigBuffer cache) {
  // Persist the freshly produced V8 code cache so subsequent navigations and
  // launches compile this preload from bytecode instead of re-parsing source.
  // Nothing the renderer sends here is trusted as cache-validation metadata:
  // SetFromRenderer() only accepts the write if |id| was served to this exact
  // frame and |source_hash| matches the hash the browser recorded for the
  // bytes it served, and it persists the entry under that browser-recorded
  // hash plus this frame's site.
  preload_code_cache::SetFromRenderer(GetRenderFrameHost(), id, source_hash,
                                      base::span<const uint8_t>(cache));
}

void ElectronWebContentsUtilityHandlerImpl::CanAccessClipboardDeprecated(
    mojom::PermissionName name,
    const blink::LocalFrameToken& frame_token,
    CanAccessClipboardDeprecatedCallback callback) {
  if (render_frame_host_token_.frame_token == frame_token) {
    // Paste requires either (1) user activation, ...
    if (web_contents()->HasRecentInteraction()) {
      std::move(callback).Run(blink::mojom::PermissionStatus::GRANTED);
      return;
    }

    // (2) granted permission, ...
    content::RenderFrameHost* render_frame_host = GetRenderFrameHost();
    content::BrowserContext* browser_context =
        render_frame_host->GetBrowserContext();
    content::PermissionController* permission_controller =
        browser_context->GetPermissionController();
    blink::PermissionType permission;
    if (name == mojom::PermissionName::DEPRECATED_SYNC_CLIPBOARD_READ) {
      permission = blink::PermissionType::DEPRECATED_SYNC_CLIPBOARD_READ;
    } else {
      std::move(callback).Run(blink::mojom::PermissionStatus::DENIED);
      return;
    }
    // TODO(wg-upgrades) https://crbug.com/406755622 remove use of deprecated
    // CreatePermissionDescriptorForPermissionType()
    blink::mojom::PermissionStatus status =
        permission_controller->GetPermissionStatusForCurrentDocument(
            content::PermissionDescriptorUtil::
                CreatePermissionDescriptorForPermissionType(permission),
            render_frame_host);
    std::move(callback).Run(status);
  } else {
    std::move(callback).Run(blink::mojom::PermissionStatus::DENIED);
  }
}

content::RenderFrameHost*
ElectronWebContentsUtilityHandlerImpl::GetRenderFrameHost() {
  return content::RenderFrameHost::FromFrameToken(render_frame_host_token_);
}

// static
void ElectronWebContentsUtilityHandlerImpl::Create(
    content::RenderFrameHost* frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronWebContentsUtility>
        receiver) {
  new ElectronWebContentsUtilityHandlerImpl(frame_host, std::move(receiver));
}
}  // namespace electron
