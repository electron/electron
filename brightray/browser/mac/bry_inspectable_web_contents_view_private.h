#import "browser/mac/bry_inspectable_web_contents_view.h"

namespace brightray {
class InspectableWebContentsViewMac;
}

@interface BRYInspectableWebContentsView (Private)

- (instancetype)initWithInspectableWebContentsViewMac:(brightray::InspectableWebContentsViewMac *)inspectableWebContentsView;
- (void)setDevToolsVisible:(BOOL)visible;
- (BOOL)isDevToolsVisible;
- (BOOL)setDockSide:(const std::string&)side;

@end
