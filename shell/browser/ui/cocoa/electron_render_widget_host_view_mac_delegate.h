// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_COCOA_ELECTRON_RENDER_WIDGET_HOST_VIEW_MAC_DELEGATE_H_
#define SHELL_BROWSER_UI_COCOA_ELECTRON_RENDER_WIDGET_HOST_VIEW_MAC_DELEGATE_H_

#import <Cocoa/Cocoa.h>

#import "content/public/browser/render_widget_host_view_mac_delegate.h"

namespace content {
class RenderWidgetHost;
}

@interface ElectronRenderWidgetHostViewMacDelegate
    : NSObject <RenderWidgetHostViewMacDelegate> {
 @private
  content::RenderWidgetHost* _renderWidgetHost;  // weak
}

- (id)initWithRenderWidgetHost:(content::RenderWidgetHost*)renderWidgetHost;
@end

#endif  // SHELL_BROWSER_UI_COCOA_ELECTRON_RENDER_WIDGET_HOST_VIEW_MAC_DELEGATE_H_
