#include "browser/win/devtools_window.h"

#include "browser/inspectable_web_contents_impl.h"
#include "browser/win/inspectable_web_contents_view_win.h"

#include "content/public/browser/web_contents_view.h"
#include "ui/base/win/hidden_window.h"

namespace brightray {

DevToolsWindow* DevToolsWindow::Create(
    InspectableWebContentsViewWin* controller) {
  return new DevToolsWindow(controller);
}

DevToolsWindow::DevToolsWindow(InspectableWebContentsViewWin* controller)
    : controller_(controller) {
}

DevToolsWindow::~DevToolsWindow() {
}

LRESULT DevToolsWindow::OnCreate(UINT, WPARAM, LPARAM) {
  auto devtools_web_contents =
      controller_->inspectable_web_contents()->devtools_web_contents();
  SetParent(devtools_web_contents->GetView()->GetNativeView(), hwnd());
  SetWindowText(hwnd(), L"Developer Tools");
  return 0;
}

LRESULT DevToolsWindow::OnDestroy(UINT, WPARAM, LPARAM) {
  auto devtools_web_contents =
      controller_->inspectable_web_contents()->devtools_web_contents();
  SetParent(
      devtools_web_contents->GetView()->GetNativeView(), ui::GetHiddenWindow());
  delete this;
  return 0;
}

LRESULT DevToolsWindow::OnSize(UINT, WPARAM, LPARAM) {
  RECT rect;
  GetClientRect(hwnd(), &rect);

  auto devtools_web_contents =
      controller_->inspectable_web_contents()->devtools_web_contents();
  SetWindowPos(devtools_web_contents->GetView()->GetNativeView(),
               nullptr,
               rect.left, rect.top,
               rect.right - rect.left, rect.bottom - rect.top,
               SWP_NOZORDER | SWP_SHOWWINDOW);

  return 0;
}

}  // namespace brightray
