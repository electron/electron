// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/content_settings_observer.h"

#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/platform/url_conversion.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace atom {

ContentSettingsObserver::ContentSettingsObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  render_frame->GetWebFrame()->SetContentSettingsClient(this);
}

ContentSettingsObserver::~ContentSettingsObserver() {}

bool ContentSettingsObserver::AllowDatabase(
    const blink::WebString& name,
    const blink::WebString& display_name,
    unsigned estimated_size) {
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

}  // namespace atom
