#include "browser/linux/inspectable_web_contents_view_linux.h"

#include "base/strings/stringprintf.h"
#include "browser/browser_client.h"
#include "browser/inspectable_web_contents_impl.h"

#include "content/public/browser/web_contents_view.h"

namespace brightray {

InspectableWebContentsView* CreateInspectableContentsView(
    InspectableWebContentsImpl* inspectable_web_contents) {
  return new InspectableWebContentsViewLinux(inspectable_web_contents);
}

InspectableWebContentsViewLinux::InspectableWebContentsViewLinux(
    InspectableWebContentsImpl* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents) {
}

InspectableWebContentsViewLinux::~InspectableWebContentsViewLinux() {
}

gfx::NativeView InspectableWebContentsViewLinux::GetNativeView() const {
  auto web_contents = inspectable_web_contents_->GetWebContents();
  return web_contents->GetView()->GetNativeView();
}

void InspectableWebContentsViewLinux::ShowDevTools() {
}

void InspectableWebContentsViewLinux::CloseDevTools() {
}

bool InspectableWebContentsViewLinux::IsDevToolsViewShowing() {
  return false;
}

void InspectableWebContentsViewLinux::SetIsDocked(bool docked) {
}

void InspectableWebContentsViewLinux::SetContentsResizingStrategy(
    const DevToolsContentsResizingStrategy& strategy) {
}

}  // namespace brightray
