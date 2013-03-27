#import "browser/inspectable_web_contents_view_mac.h"

#import "browser/inspectable_web_contents.h"
#import "browser/mac/bry_inspectable_web_contents_view_private.h"

#import "content/public/browser/web_contents_view.h"
#import <AppKit/AppKit.h>

namespace brightray {
  
InspectableWebContentsView* CreateInspectableContentsView(InspectableWebContentsImpl* inspectable_web_contents) {
  return new InspectableWebContentsViewMac(inspectable_web_contents);
}

InspectableWebContentsViewMac::InspectableWebContentsViewMac(InspectableWebContentsImpl* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents),
      view_([[BRYInspectableWebContentsView alloc] initWithInspectableWebContentsViewMac:this]) {
}

gfx::NativeView InspectableWebContentsViewMac::GetNativeView() const {
  return view_.get();
}

void InspectableWebContentsViewMac::ShowDevTools() {
  [view_ setDevToolsVisible:YES];
}

void InspectableWebContentsViewMac::CloseDevTools() {
  [view_ setDevToolsVisible:NO];
}

}
