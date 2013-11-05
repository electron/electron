#include "browser/win/inspectable_web_contents_view_win.h"

#include "browser/browser_client.h"
#include "browser/inspectable_web_contents_impl.h"
#include "browser/win/devtools_window.h"

#include "content/public/browser/web_contents_view.h"
#include "ui/gfx/win/hwnd_util.h"

namespace brightray {

namespace {

const int kWindowInset = 100;

}

InspectableWebContentsView* CreateInspectableContentsView(
    InspectableWebContentsImpl* inspectable_web_contents) {
  return new InspectableWebContentsViewWin(inspectable_web_contents);
}

InspectableWebContentsViewWin::InspectableWebContentsViewWin(
    InspectableWebContentsImpl* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents) {
}

InspectableWebContentsViewWin::~InspectableWebContentsViewWin() {
  if (devtools_window_)
    DestroyWindow(devtools_window_->hwnd());
}

gfx::NativeView InspectableWebContentsViewWin::GetNativeView() const {
  auto web_contents = inspectable_web_contents_->GetWebContents();
  return web_contents->GetView()->GetNativeView();
}

void InspectableWebContentsViewWin::ShowDevTools() {
  if (!devtools_window_) {
    devtools_window_ = DevToolsWindow::Create(this)->AsWeakPtr();
    devtools_window_->Init(HWND_DESKTOP, gfx::Rect());
  }

  auto contents_view = inspectable_web_contents_->GetWebContents()->GetView();
  auto size = contents_view->GetContainerSize();
  size.Enlarge(-kWindowInset, -kWindowInset);
  gfx::CenterAndSizeWindow(contents_view->GetNativeView(),
                           devtools_window_->hwnd(),
                           size);

  ShowWindow(devtools_window_->hwnd(), SW_SHOWNORMAL);
}

void InspectableWebContentsViewWin::CloseDevTools() {
  if (!devtools_window_)
    return;
  SendMessage(devtools_window_->hwnd(), WM_CLOSE, 0, 0);
}

bool InspectableWebContentsViewWin::IsDevToolsOpened() {
  return devtools_window_;
}

bool InspectableWebContentsViewWin::SetDockSide(const std::string& side) {
  return false;
}

}  // namespace brightray
