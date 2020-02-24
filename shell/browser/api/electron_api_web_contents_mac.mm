// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "content/public/browser/render_widget_host_view.h"
#include "shell/browser/api/electron_api_web_contents.h"

#import <Cocoa/Cocoa.h>

namespace electron {

namespace api {

bool WebContents::IsFocused() const {
  auto* view = web_contents()->GetRenderWidgetHostView();
  if (!view)
    return false;

  if (GetType() != Type::BACKGROUND_PAGE) {
    auto window = [web_contents()->GetNativeView().GetNativeNSView() window];
    // On Mac the render widget host view does not lose focus when the window
    // loses focus so check if the top level window is the key window.
    if (window && ![window isKeyWindow])
      return false;
  }

  return view->HasFocus();
}

}  // namespace api

}  // namespace electron
