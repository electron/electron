// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/atom_extension_host_delegate.h"

#include <memory>
#include <string>
#include <utility>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "extensions/browser/media_capture_util.h"
#include "shell/browser/extensions/atom_extension_web_contents_observer.h"

namespace extensions {

AtomExtensionHostDelegate::AtomExtensionHostDelegate() {}

AtomExtensionHostDelegate::~AtomExtensionHostDelegate() {}

void AtomExtensionHostDelegate::OnExtensionHostCreated(
    content::WebContents* web_contents) {
  AtomExtensionWebContentsObserver::CreateForWebContents(web_contents);
}

void AtomExtensionHostDelegate::OnRenderViewCreatedForBackgroundPage(
    ExtensionHost* host) {}

content::JavaScriptDialogManager*
AtomExtensionHostDelegate::GetJavaScriptDialogManager() {
  // TODO(jamescook): Create a JavaScriptDialogManager or reuse the one from
  // content_shell.
  NOTREACHED();
  return nullptr;
}

void AtomExtensionHostDelegate::CreateTab(
    std::unique_ptr<content::WebContents> web_contents,
    const std::string& extension_id,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_rect,
    bool user_gesture) {
  // TODO(jamescook): Should app_shell support opening popup windows?
  NOTREACHED();
}

void AtomExtensionHostDelegate::ProcessMediaAccessRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback,
    const Extension* extension) {
  // Allow access to the microphone and/or camera.
  media_capture_util::GrantMediaStreamRequest(web_contents, request,
                                              std::move(callback), extension);
}

bool AtomExtensionHostDelegate::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    blink::mojom::MediaStreamType type,
    const Extension* extension) {
  media_capture_util::VerifyMediaAccessPermission(type, extension);
  return true;
}

content::PictureInPictureResult
AtomExtensionHostDelegate::EnterPictureInPicture(
    content::WebContents* web_contents,
    const viz::SurfaceId& surface_id,
    const gfx::Size& natural_size) {
  NOTREACHED();
  return content::PictureInPictureResult();
}

void AtomExtensionHostDelegate::ExitPictureInPicture() {
  NOTREACHED();
}

}  // namespace extensions
