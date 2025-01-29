// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_CONTENT_SETTINGS_OBSERVER_H_
#define ELECTRON_SHELL_RENDERER_CONTENT_SETTINGS_OBSERVER_H_

#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "shell/common/web_contents_utility.mojom.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "url/origin.h"

namespace electron {

class ContentSettingsObserver : public content::RenderFrameObserver,
                                public blink::WebContentSettingsClient {
 public:
  explicit ContentSettingsObserver(content::RenderFrame* render_frame);
  ~ContentSettingsObserver() override;

  // disable copy
  ContentSettingsObserver(const ContentSettingsObserver&) = delete;
  ContentSettingsObserver& operator=(const ContentSettingsObserver&) = delete;

  // blink::WebContentSettingsClient implementation.
  bool AllowStorageAccessSync(StorageType storage_type) override;
  bool AllowReadFromClipboardSync() override;

 private:
  // content::RenderFrameObserver implementation.
  void OnDestruct() override;

  // A getter for `content_settings_manager_` that ensures it is bound.
  mojom::ElectronWebContentsUtility& GetWebContentsUtility();

  mojo::AssociatedRemote<mojom::ElectronWebContentsUtility>
      web_contents_utility_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_CONTENT_SETTINGS_OBSERVER_H_
