#include "browser/win/inspectable_web_contents_view_win.h"

#include "browser/browser_client.h"
#include "browser/inspectable_web_contents_impl.h"
#include "browser/win/devtools_window.h"

#include "content/public/browser/web_contents_view.h"
#include "ui/gfx/win/hwnd_util.h"
#include "ui/views/controls/single_split_view.h"
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
        dockside_("none"),  // "none" is treated as "bottom".
        web_view_(new views::WebView(NULL)),
        web_contents_view_(web_contents_view) {
    set_owned_by_client();
    web_view_->set_owned_by_client();
    web_view_->SetWebContents(
        web_contents_view_->inspectable_web_contents()->GetWebContents());
  }

  views::View* GetWebView() const {
    return web_view_.get();
  }

  void ShowDevTools() {
    if (IsDevToolsViewShowing())
      return;

    RemoveChildView(web_view_.get());
    devtools_view_ = new views::WebView(NULL);
    devtools_view_->SetWebContents(web_contents_view_->
        inspectable_web_contents()->devtools_web_contents());
    split_view_.reset(new views::SingleSplitView(
        web_view_.get(),
        devtools_view_,
        GetSplitViewOrientation(),
        NULL));
    split_view_->set_divider_offset(GetSplitVievDividerOffset());
    AddChildView(split_view_.get());
    Layout();

    devtools_view_->RequestFocus();
  }

  void CloseDevTools() {
    if (!IsDevToolsViewShowing())
      return;

    RemoveChildView(split_view_.get());
    split_view_.reset();
    AddChildView(web_view_.get());
    Layout();
  }

  bool IsDevToolsViewShowing() {
    return split_view_;
  }

  void SetDockSide(const std::string& side) {
    if (dockside_ == side)
      return;  // no change from current location

    dockside_ = side;
    if (!IsDevToolsViewShowing())
      return;

    split_view_->set_orientation(GetSplitViewOrientation());
    split_view_->set_divider_offset(GetSplitVievDividerOffset());
    split_view_->Layout();
    return;
  }

 private:
  // views::View:
  virtual void Layout() OVERRIDE {
    if (split_view_)
      split_view_->SetBounds(0, 0, width(), height());
    else
      web_view_->SetBounds(0, 0, width(), height());
  }

  virtual void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) OVERRIDE {
    View::ViewHierarchyChanged(details);
    // We're not using child == this because a Widget may not be
    // available when this is added to the hierarchy.
    if (details.is_add && GetWidget() && !container_view_created_) {
      container_view_created_ = true;
      AddChildView(web_view_.get());
    }
  }

  views::SingleSplitView::Orientation GetSplitViewOrientation() const {
    if (dockside_ == "right")
      return views::SingleSplitView::HORIZONTAL_SPLIT;
    else
      return views::SingleSplitView::VERTICAL_SPLIT;
  }

  int GetSplitVievDividerOffset() const {
    if (dockside_ == "right")
      return width() * 2 / 3;
    else
      return height() * 2 / 3;
  }

  // True if the container view has already been created, or false otherwise.
  bool container_view_created_;

  std::string dockside_;

  scoped_ptr<views::WebView> web_view_;
  scoped_ptr<views::SingleSplitView> split_view_;
  views::WebView* devtools_view_;  // Owned by split_view_.

  InspectableWebContentsViewWin* web_contents_view_;
};

InspectableWebContentsView* CreateInspectableContentsView(
    InspectableWebContentsImpl* inspectable_web_contents) {
  return new InspectableWebContentsViewWin(inspectable_web_contents);
}

InspectableWebContentsViewWin::InspectableWebContentsViewWin(
    InspectableWebContentsImpl* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents),
      undocked_(false),
      container_(new ContainerView(this)) {
}

InspectableWebContentsViewWin::~InspectableWebContentsViewWin() {
  if (devtools_window_)
    DestroyWindow(devtools_window_->hwnd());
}

views::View* InspectableWebContentsViewWin::GetView() const {
  return container_.get();
}

views::View* InspectableWebContentsViewWin::GetWebView() const {
  return container_->GetWebView();
}

gfx::NativeView InspectableWebContentsViewWin::GetNativeView() const {
  auto web_contents = inspectable_web_contents_->GetWebContents();
  return web_contents->GetView()->GetNativeView();
}

void InspectableWebContentsViewWin::ShowDevTools() {
  if (undocked_) {
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
  } else {
    container_->ShowDevTools();
  }
}

void InspectableWebContentsViewWin::CloseDevTools() {
  if (undocked_)
    SendMessage(devtools_window_->hwnd(), WM_CLOSE, 0, 0);
  else
    container_->CloseDevTools();
}

bool InspectableWebContentsViewWin::IsDevToolsViewShowing() {
  return container_->IsDevToolsViewShowing() || devtools_window_;
}

bool InspectableWebContentsViewWin::SetDockSide(const std::string& side) {
  if (side == "undocked") {
    undocked_ = true;
    container_->CloseDevTools();
  } else if (side == "right" || side == "bottom") {
    undocked_ = false;
    if (devtools_window_)
      SendMessage(devtools_window_->hwnd(), WM_CLOSE, 0, 0);
    container_->SetDockSide(side);
  } else {
    return false;
  }

  ShowDevTools();
  return true;
}

}  // namespace brightray
