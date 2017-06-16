// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_CONTENT_SETTINGS_OBSERVER_H_
#define ATOM_RENDERER_CONTENT_SETTINGS_OBSERVER_H_

#include "base/compiler_specific.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/WebKit/public/platform/WebContentSettingsClient.h"

namespace atom {

class ContentSettingsObserver : public content::RenderFrameObserver,
                                public blink::WebContentSettingsClient {
 public:
  explicit ContentSettingsObserver(content::RenderFrame* render_frame);
  ~ContentSettingsObserver() override;

  // blink::WebContentSettingsClient implementation.
  bool AllowDatabase(const blink::WebString& name,
                     const blink::WebString& display_name,
                     unsigned estimated_size) override;
  bool AllowStorage(bool local) override;
  bool AllowIndexedDB(const blink::WebString& name,
                      const blink::WebSecurityOrigin& security_origin) override;

 private:
  // content::RenderFrameObserver implementation.
  void OnDestruct() override;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingsObserver);
};

}  // namespace atom

#endif  // ATOM_RENDERER_CONTENT_SETTINGS_OBSERVER_H_
