// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/shell_extension_host_delegate.h"

#include <memory>
#include <string>
#include <utility>

#include "atom/browser/extensions/shell_extension_web_contents_observer.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "extensions/browser/media_capture_util.h"
#include "extensions/browser/serial_extension_host_queue.h"

namespace extensions {

ShellExtensionHostDelegate::ShellExtensionHostDelegate() {}

ShellExtensionHostDelegate::~ShellExtensionHostDelegate() {}

void ShellExtensionHostDelegate::OnExtensionHostCreated(
    content::WebContents* web_contents) {
  ShellExtensionWebContentsObserver::CreateForWebContents(web_contents);
}

void ShellExtensionHostDelegate::OnRenderViewCreatedForBackgroundPage(
    ExtensionHost* host) {}

content::JavaScriptDialogManager*
ShellExtensionHostDelegate::GetJavaScriptDialogManager() {
  // TODO(jamescook): Create a JavaScriptDialogManager or reuse the one from
  // content_shell.
  NOTREACHED();
  return NULL;
}

void ShellExtensionHostDelegate::CreateTab(
    std::unique_ptr<content::WebContents> web_contents,
    const std::string& extension_id,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_rect,
    bool user_gesture) {
  // TODO(jamescook): Should app_shell support opening popup windows?
  NOTREACHED();
}

void ShellExtensionHostDelegate::ProcessMediaAccessRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback,
    const Extension* extension) {
  // Allow access to the microphone and/or camera.
  media_capture_util::GrantMediaStreamRequest(web_contents, request,
                                              std::move(callback), extension);
}

bool ShellExtensionHostDelegate::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    blink::MediaStreamType type,
    const Extension* extension) {
  media_capture_util::VerifyMediaAccessPermission(type, extension);
  return true;
}

static base::LazyInstance<SerialExtensionHostQueue>::DestructorAtExit g_queue =
    LAZY_INSTANCE_INITIALIZER;

ExtensionHostQueue* ShellExtensionHostDelegate::GetExtensionHostQueue() const {
  return g_queue.Pointer();
}

gfx::Size ShellExtensionHostDelegate::EnterPictureInPicture(
    content::WebContents* web_contents,
    const viz::SurfaceId& surface_id,
    const gfx::Size& natural_size) {
  NOTREACHED();
  return gfx::Size();
}

void ShellExtensionHostDelegate::ExitPictureInPicture() {
  NOTREACHED();
}

}  // namespace extensions
