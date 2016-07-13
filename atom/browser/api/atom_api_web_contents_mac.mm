// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

@interface NSWindow
- (BOOL)isKeyWindow;
@end

namespace atom {

namespace api {

bool WebContents::IsFocused() const {
  if (GetType() != BACKGROUND_PAGE) {
    auto window = web_contents()->GetTopLevelNativeWindow();
    // On Mac the render widget host view does not lose focus when the window
    // loses focus so check if the top level window is the key window.
    if (window && ![window isKeyWindow])
      return false;
  }

  auto view = web_contents()->GetRenderWidgetHostView();
  return view && view->HasFocus();
}

}  // namespace api

}  // namespace atom
