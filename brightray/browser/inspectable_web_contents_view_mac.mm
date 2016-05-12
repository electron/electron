#include "browser/inspectable_web_contents_view_mac.h"

#import <AppKit/AppKit.h>

#include "base/strings/sys_string_conversions.h"
#include "browser/inspectable_web_contents.h"
#include "browser/inspectable_web_contents_view_delegate.h"
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
  [view_ removeObservers];
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

bool InspectableWebContentsViewMac::IsDevToolsViewFocused() {
  return [view_ isDevToolsFocused];
}

void InspectableWebContentsViewMac::SetIsDocked(bool docked) {
  [view_ setIsDocked:docked];
}

void InspectableWebContentsViewMac::SetContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy) {
  [view_ setContentsResizingStrategy:strategy];
}

void InspectableWebContentsViewMac::SetTitle(const base::string16& title) {
  [view_ setTitle:base::SysUTF16ToNSString(title)];
}

}
