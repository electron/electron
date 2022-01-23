#include "shell/browser/ui/native_wrapper_browser_view.h"

#include "shell/browser/api/electron_api_browser_view.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "ui/views/view.h"

namespace electron {

void NativeWrapperBrowserView::InitWrapperBrowserView() {
  SetNativeView(new views::View());
}

void NativeWrapperBrowserView::SetBrowserViewImpl() {
  if (!GetNative())
    return;
  if (api_browser_view_->view()->GetInspectableWebContentsView()) {
    GetNative()->AddChildView(
        api_browser_view_->view()->GetInspectableWebContentsView()->GetView());
  }
}

void NativeWrapperBrowserView::DetachBrowserViewImpl() {
  if (!GetNative())
    return;
  if (api_browser_view_->view()->GetInspectableWebContentsView()) {
    GetNative()->RemoveChildView(
        api_browser_view_->view()->GetInspectableWebContentsView()->GetView());
  }
}

}  // namespace electron
