#include "shell/browser/ui/native_wrapper_browser_view.h"

#include "shell/browser/api/electron_api_browser_view.h"
#include "shell/browser/ui/cocoa/electron_native_view.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"

namespace electron {

void NativeWrapperBrowserView::InitWrapperBrowserView() {
  SetNativeView([[ElectronNativeView alloc] init]);
}

void NativeWrapperBrowserView::SetBrowserViewImpl() {
  if (api_browser_view_->view()->GetInspectableWebContentsView()) {
    auto* view = api_browser_view_->view()
                     ->GetInspectableWebContentsView()
                     ->GetNativeView()
                     .GetNativeNSView();
    [GetNative() addSubview:view];
  }
}

void NativeWrapperBrowserView::DetachBrowserViewImpl() {
  if (api_browser_view_->view()->GetInspectableWebContentsView()) {
    auto* view = api_browser_view_->view()
                     ->GetInspectableWebContentsView()
                     ->GetNativeView()
                     .GetNativeNSView();
    [view removeFromSuperview];
  }
}

void NativeWrapperBrowserView::UpdateDraggableRegions() {
  if (api_browser_view_) {
    api_browser_view_->view()->UpdateDraggableRegions(
        api_browser_view_->view()->GetDraggableRegions());
  }
}

}  // namespace electron
