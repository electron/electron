// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_contents.h"

#include "content/browser/renderer_host/frame_tree.h"        // nogncheck
#include "content/browser/renderer_host/frame_tree_node.h"   // nogncheck
#include "content/browser/web_contents/web_contents_impl.h"  // nogncheck
#include "shell/browser/osr/osr_render_widget_host_view.h"
#include "shell/browser/osr/osr_web_contents_view.h"

// Including both web_contents_impl.h and node.h would introduce a error, we
// have to isolate the usage of WebContentsImpl into a clean file to fix it:
// error C2371: 'ssize_t': redefinition; different basic types

namespace electron::api {

void WebContents::DetachFromOuterFrame() {
  // See detach_webview_frame.patch on how to detach.
  content::FrameTreeNodeId frame_tree_node_id =
      static_cast<content::WebContentsImpl*>(web_contents())
          ->GetOuterDelegateFrameTreeNodeId();
  if (!frame_tree_node_id) {
    auto* node = content::FrameTreeNode::GloballyFindByID(frame_tree_node_id);
    DCHECK(node->parent());
    node->frame_tree().RemoveFrame(node);
  }
}

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
