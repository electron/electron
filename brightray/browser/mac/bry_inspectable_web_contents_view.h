#import <AppKit/AppKit.h>

#include <string>

#include "base/mac/scoped_nsobject.h"

namespace brightray {
class InspectableWebContentsViewMac;
}

@interface BRYInspectableWebContentsView : NSView<NSWindowDelegate> {
@private
  brightray::InspectableWebContentsViewMac* inspectableWebContentsView_;
  base::scoped_nsobject<NSWindow> devtools_window_;
  BOOL devtools_visible_;
}

- (instancetype)initWithInspectableWebContentsViewMac:(brightray::InspectableWebContentsViewMac*)inspectableWebContentsView;
- (IBAction)showDevTools:(id)sender;
- (void)setDevToolsVisible:(BOOL)visible;
- (BOOL)isDevToolsVisible;
- (BOOL)setDockSide:(const std::string&)side ;

@end
