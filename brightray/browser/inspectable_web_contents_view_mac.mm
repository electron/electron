#include "browser/inspectable_web_contents_view_mac.h"

#import <AppKit/AppKit.h>

#include "browser/inspectable_web_contents.h"
#import "browser/mac/bry_inspectable_web_contents_view.h"

namespace brightray {

InspectableWebContentsView* CreateInspectableContentsView(InspectableWebContentsImpl* inspectable_web_contents) {
  return new InspectableWebContentsViewMac(inspectable_web_contents);
}

InspectableWebContentsViewMac::InspectableWebContentsViewMac(InspectableWebContentsImpl* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents),
      view_([[BRYInspectableWebContentsView alloc] initWithInspectableWebContentsViewMac:this]) {
}

InspectableWebContentsViewMac::~InspectableWebContentsViewMac() {
  CloseDevTools();
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

bool InspectableWebContentsViewMac::IsDevToolsViewShowing() {
  return [view_ isDevToolsVisible];
}

void InspectableWebContentsViewMac::SetIsDocked(bool docked) {
  [view_ setIsDocked:docked];
}

void InspectableWebContentsViewMac::SetContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy) {
  [view_ setContentsResizingStrategy:strategy];
}

}
