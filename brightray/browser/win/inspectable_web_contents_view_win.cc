#include "browser/win/inspectable_web_contents_view_win.h"

#include "browser/inspectable_web_contents_impl.h"

#include "content/public/browser/web_contents_view.h"

namespace brightray {

InspectableWebContentsView* CreateInspectableContentsView(InspectableWebContentsImpl* inspectable_web_contents) {
  return new InspectableWebContentsViewWin(inspectable_web_contents);
}

InspectableWebContentsViewWin::InspectableWebContentsViewWin(InspectableWebContentsImpl* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents) {
}

InspectableWebContentsViewWin::~InspectableWebContentsViewWin() {
}

gfx::NativeView InspectableWebContentsViewWin::GetNativeView() const {
  return inspectable_web_contents_->GetWebContents()->GetView()->GetNativeView();
}

void InspectableWebContentsViewWin::ShowDevTools() {
}

void InspectableWebContentsViewWin::CloseDevTools() {
}

bool InspectableWebContentsViewWin::SetDockSide(const std::string& side) {
  return false;
}

}
