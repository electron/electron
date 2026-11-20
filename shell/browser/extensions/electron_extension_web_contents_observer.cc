// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extension_web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/common/mojom/frame.mojom.h"
#include "shell/browser/extensions/electron_extension_tab_registry.h"

namespace extensions {

ElectronExtensionWebContentsObserver::ElectronExtensionWebContentsObserver(
    content::WebContents* web_contents)
    : ExtensionWebContentsObserver(web_contents),
      content::WebContentsUserData<ElectronExtensionWebContentsObserver>(
          *web_contents) {}

ElectronExtensionWebContentsObserver::~ElectronExtensionWebContentsObserver() =
    default;

void ElectronExtensionWebContentsObserver::CreateForWebContents(
    content::WebContents* web_contents) {
  content::WebContentsUserData<
      ElectronExtensionWebContentsObserver>::CreateForWebContents(web_contents);

  // Initialize this instance if necessary.
  FromWebContents(web_contents)->Initialize();
}

void ElectronExtensionWebContentsObserver::InitializeRenderFrame(
    content::RenderFrameHost* render_frame_host) {
  ExtensionWebContentsObserver::InitializeRenderFrame(render_frame_host);

  // Frames created after Extensions.addView() learn their window here; the
  // registry tells the frames that already exist.
  const auto* data = ElectronExtensionTabData::FromWebContents(web_contents());
  if (!data)
    return;
  if (auto* local_frame = GetLocalFrame(render_frame_host))
    local_frame->UpdateBrowserWindowId(data->window_id());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ElectronExtensionWebContentsObserver);

}  // namespace extensions
