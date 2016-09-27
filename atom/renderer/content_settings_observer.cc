// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/content_settings_observer.h"

#include "content/public/renderer/render_frame.h"
#include "third_party/WebKit/public/platform/URLConversion.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace atom {

ContentSettingsObserver::ContentSettingsObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  render_frame->GetWebFrame()->setContentSettingsClient(this);
}

ContentSettingsObserver::~ContentSettingsObserver() {
}

bool ContentSettingsObserver::allowDatabase(
    const blink::WebString& name,
    const blink::WebString& display_name,
    unsigned estimated_size) {
  blink::WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->getSecurityOrigin().isUnique() ||
      frame->top()->getSecurityOrigin().isUnique())
    return false;
  auto origin = blink::WebStringToGURL(frame->getSecurityOrigin().toString());
  if (!origin.IsStandard())
    return false;
  return true;
}

bool ContentSettingsObserver::allowStorage(bool local) {
  blink::WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->getSecurityOrigin().isUnique() ||
      frame->top()->getSecurityOrigin().isUnique())
    return false;
  auto origin = blink::WebStringToGURL(frame->getSecurityOrigin().toString());
  if (!origin.IsStandard())
    return false;
  return true;
}

bool ContentSettingsObserver::allowIndexedDB(
    const blink::WebString& name,
    const blink::WebSecurityOrigin& security_origin) {
  blink::WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->getSecurityOrigin().isUnique() ||
      frame->top()->getSecurityOrigin().isUnique())
    return false;
  auto origin = blink::WebStringToGURL(frame->getSecurityOrigin().toString());
  if (!origin.IsStandard())
    return false;
  return true;
}

void ContentSettingsObserver::OnDestruct() {
  delete this;
}

}  // namespace atom
