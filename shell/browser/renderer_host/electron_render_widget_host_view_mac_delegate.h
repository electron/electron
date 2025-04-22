// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_RENDERER_HOST_CHROME_RENDER_WIDGET_HOST_VIEW_MAC_DELEGATE_H_
#define ELECTRON_BROWSER_RENDERER_HOST_CHROME_RENDER_WIDGET_HOST_VIEW_MAC_DELEGATE_H_

#import <Cocoa/Cocoa.h>

#import "content/public/browser/render_widget_host_view_mac_delegate.h"

namespace content {
class RenderWidgetHost;
}

@interface ElectronRenderWidgetHostViewMacDelegate
    : NSObject <RenderWidgetHostViewMacDelegate>

- (instancetype)initWithRenderWidgetHost:
    (content::RenderWidgetHost*)renderWidgetHost;

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item
                      isValidItem:(BOOL*)valid;
@end

#endif  // ELECTRON_BROWSER_RENDERER_HOST_CHROME_RENDER_WIDGET_HOST_VIEW_MAC_DELEGATE_H_
