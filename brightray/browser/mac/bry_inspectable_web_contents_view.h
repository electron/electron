#import <AppKit/AppKit.h>

#include "browser/devtools_contents_resizing_strategy.h"

#include "base/mac/scoped_nsobject.h"
#include "ui/base/cocoa/base_view.h"

namespace brightray {
class InspectableWebContentsViewMac;
}

using brightray::InspectableWebContentsViewMac;

@interface BRYInspectableWebContentsView : BaseView<NSWindowDelegate> {
@private
  brightray::InspectableWebContentsViewMac* inspectableWebContentsView_;

  base::scoped_nsobject<NSWindow> devtools_window_;
  BOOL devtools_visible_;
  BOOL devtools_docked_;

  DevToolsContentsResizingStrategy strategy_;
}

- (instancetype)initWithInspectableWebContentsViewMac:(InspectableWebContentsViewMac*)view;
- (void)setDevToolsVisible:(BOOL)visible;
- (BOOL)isDevToolsVisible;
- (void)setIsDocked:(BOOL)docked;
- (void)setContentsResizingStrategy:(const DevToolsContentsResizingStrategy&)strategy;

@end
