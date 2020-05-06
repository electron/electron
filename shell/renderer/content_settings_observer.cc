// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/content_settings_observer.h"

#include "content/public/renderer/render_frame.h"
#include "shell/common/options_switches.h"
#include "third_party/blink/public/platform/url_conversion.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

ContentSettingsObserver::ContentSettingsObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  render_frame->GetWebFrame()->SetContentSettingsClient(this);
}

ContentSettingsObserver::~ContentSettingsObserver() {}

bool ContentSettingsObserver::AllowDatabase() {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableWebSQL)) {
    return false;
  }

  blink::WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->GetSecurityOrigin().IsUnique() ||
      frame->Top()->GetSecurityOrigin().IsUnique())
    return false;
  auto origin = blink::WebStringToGURL(frame->GetSecurityOrigin().ToString());
  if (!origin.IsStandard())
    return false;
  return true;
}

bool ContentSettingsObserver::AllowStorage(bool local) {
  blink::WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->GetSecurityOrigin().IsUnique() ||
      frame->Top()->GetSecurityOrigin().IsUnique())
    return false;
  auto origin = blink::WebStringToGURL(frame->GetSecurityOrigin().ToString());
  if (!origin.IsStandard())
    return false;
  return true;
}

bool ContentSettingsObserver::AllowIndexedDB(
    const blink::WebSecurityOrigin& security_origin) {
  blink::WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->GetSecurityOrigin().IsUnique() ||
      frame->Top()->GetSecurityOrigin().IsUnique())
    return false;
  auto origin = blink::WebStringToGURL(frame->GetSecurityOrigin().ToString());
  if (!origin.IsStandard())
    return false;
  return true;
}

void ContentSettingsObserver::OnDestruct() {
  delete this;
}

}  // namespace electron
