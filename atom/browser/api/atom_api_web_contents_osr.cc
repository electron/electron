// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include "atom/browser/osr/osr_render_widget_host_view.h"
#include "atom/browser/osr/osr_web_contents_view.h"
#include "content/browser/web_contents/web_contents_impl.h"

// Including both web_contents_impl.h and node.h would introduce a error, we
// have to isolate the usage of WebContentsImpl into a clean file to fix it:
// error C2371: 'ssize_t': redefinition; different basic types

namespace atom {

namespace api {

OffScreenWebContentsView* WebContents::GetOffScreenWebContentsView() const {
  const auto* impl =
      static_cast<const content::WebContentsImpl*>(web_contents());
  return static_cast<OffScreenWebContentsView*>(impl->GetView());
}

OffScreenRenderWidgetHostView* WebContents::GetOffScreenRenderWidgetHostView()
    const {
  return static_cast<OffScreenRenderWidgetHostView*>(
      web_contents()->GetRenderWidgetHostView());
}

}  // namespace api

}  // namespace atom
