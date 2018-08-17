#import <AppKit/AppKit.h>

#include "base/mac/scoped_nsobject.h"
#include "brightray/browser/devtools_contents_resizing_strategy.h"
#include "ui/base/cocoa/base_view.h"

namespace brightray {
class InspectableWebContentsViewMac;
}

using brightray::InspectableWebContentsViewMac;

@interface BRYInspectableWebContentsView : BaseView <NSWindowDelegate> {
 @private
  brightray::InspectableWebContentsViewMac* inspectableWebContentsView_;

  base::scoped_nsobject<NSView> fake_view_;
  base::scoped_nsobject<NSWindow> devtools_window_;
  BOOL devtools_visible_;
  BOOL devtools_docked_;
  BOOL devtools_is_first_responder_;

  DevToolsContentsResizingStrategy strategy_;
}

- (instancetype)initWithInspectableWebContentsViewMac:
    (InspectableWebContentsViewMac*)view;
- (void)removeObservers;
- (void)notifyDevToolsFocused;
- (void)setDevToolsVisible:(BOOL)visible;
- (BOOL)isDevToolsVisible;
- (BOOL)isDevToolsFocused;
- (void)setIsDocked:(BOOL)docked;
- (void)setContentsResizingStrategy:
    (const DevToolsContentsResizingStrategy&)strategy;
- (void)setTitle:(NSString*)title;

@end
