#include "browser/win/inspectable_web_contents_view_win.h"

#include "browser/browser_client.h"
#include "browser/inspectable_web_contents_impl.h"
#include "browser/win/devtools_window.h"

#include "content/public/browser/web_contents_view.h"
#include "ui/gfx/win/hwnd_util.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/view.h"

namespace brightray {

namespace {

const int kWindowInset = 100;

}

class ContainerView : public views::View {
 public:
  explicit ContainerView(InspectableWebContentsViewWin* web_contents_view)
      : container_view_created_(false),
        web_view_(new views::WebView(NULL)),
        web_contents_view_(web_contents_view) {
    web_view_->SetWebContents(
        web_contents_view_->inspectable_web_contents()->GetWebContents());
  }

 private:
  // views::View:
  virtual void Layout() OVERRIDE {
    web_view_->SetBounds(0, 0, width(), height());
  }

  virtual void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) OVERRIDE {
    View::ViewHierarchyChanged(details);
    // We're not using child == this because a Widget may not be
    // available when this is added to the hierarchy.
    if (details.is_add && GetWidget() && !container_view_created_) {
      container_view_created_ = true;
      AddChildView(web_view_);
    }
  }

  // True if the container view has already been created, or false otherwise.
  bool container_view_created_;

  views::WebView* web_view_;

  InspectableWebContentsViewWin* web_contents_view_;
};

InspectableWebContentsView* CreateInspectableContentsView(
    InspectableWebContentsImpl* inspectable_web_contents) {
  return new InspectableWebContentsViewWin(inspectable_web_contents);
}

InspectableWebContentsViewWin::InspectableWebContentsViewWin(
    InspectableWebContentsImpl* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents),
      container_(new ContainerView(this)) {
}

InspectableWebContentsViewWin::~InspectableWebContentsViewWin() {
  if (devtools_window_)
    DestroyWindow(devtools_window_->hwnd());
}

views::View* InspectableWebContentsViewWin::GetView() const {
  return container_;
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

bool InspectableWebContentsViewWin::IsDevToolsViewShowing() {
  return devtools_window_;
}

bool InspectableWebContentsViewWin::SetDockSide(const std::string& side) {
  return false;
}

}  // namespace brightray
