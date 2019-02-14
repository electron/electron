// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/renderer_preferences.h"

#import <Cocoa/Cocoa.h>

namespace atom {

namespace api {

bool WebContents::IsFocused() const {
  auto* view = web_contents()->GetRenderWidgetHostView();
  if (!view)
    return false;

  if (GetType() != BACKGROUND_PAGE) {
    auto window = [web_contents()->GetNativeView().GetNativeNSView() window];
    // On Mac the render widget host view does not lose focus when the window
    // loses focus so check if the top level window is the key window.
    if (window && ![window isKeyWindow])
      return false;
  }

  return view->HasFocus();
}

void WebContents::SetAcceptLanguages(content::RendererPreferences* prefs) {
  prefs->accept_languages = base::SysNSStringToUTF8(
      [[NSLocale preferredLanguages] componentsJoinedByString:@","]);
}

}  // namespace api

}  // namespace atom
