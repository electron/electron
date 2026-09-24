// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_contents.h"

#include "content/browser/web_contents/web_contents_impl.h"  // nogncheck
#include "shell/browser/osr/osr_render_widget_host_view.h"
#include "shell/browser/osr/osr_web_contents_view.h"

// Including both web_contents_impl.h and node.h would introduce a error, we
// have to isolate the usage of WebContentsImpl into a clean file to fix it:
// error C2371: 'ssize_t': redefinition; different basic types

namespace electron::api {

OffScreenWebContentsView* WebContents::GetOffScreenWebContentsView() const {
  if (IsOffScreen()) {
    const auto* impl =
        static_cast<const content::WebContentsImpl*>(web_contents());
    return static_cast<OffScreenWebContentsView*>(impl->GetView());
  } else {
    return nullptr;
  }
}

OffScreenRenderWidgetHostView* WebContents::GetOffScreenRenderWidgetHostView()
    const {
  if (IsOffScreen()) {
    return static_cast<OffScreenRenderWidgetHostView*>(
        web_contents()->GetRenderWidgetHostView());
  } else {
    return nullptr;
  }
}

}  // namespace electron::api
