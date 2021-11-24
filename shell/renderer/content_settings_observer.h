// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_CONTENT_SETTINGS_OBSERVER_H_
#define ELECTRON_SHELL_RENDERER_CONTENT_SETTINGS_OBSERVER_H_

#include "base/compiler_specific.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"

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

 private:
  // content::RenderFrameObserver implementation.
  void OnDestruct() override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_CONTENT_SETTINGS_OBSERVER_H_
