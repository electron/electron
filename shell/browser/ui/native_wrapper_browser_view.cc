#include "shell/browser/ui/native_wrapper_browser_view.h"

#include "content/public/browser/web_contents.h"
#include "shell/browser/api/electron_api_browser_view.h"
#include "shell/browser/api/electron_api_web_contents.h"

namespace electron {

NativeWrapperBrowserView::NativeWrapperBrowserView() {
  InitWrapperBrowserView();
}

NativeWrapperBrowserView::~NativeWrapperBrowserView() = default;

void NativeWrapperBrowserView::SetBrowserView(api::BrowserView* browser_view) {
  api_browser_view_ = browser_view;
  if (api_browser_view_)
    SetBrowserViewImpl();
}

void NativeWrapperBrowserView::SetBounds(const gfx::Rect& bounds) {
  NativeView::SetBounds(bounds);
  if (api_browser_view_)
    api_browser_view_->view()->SetBounds(
        gfx::Rect(0, 0, bounds.width(), bounds.height()));
}

void NativeWrapperBrowserView::DetachBrowserView(NativeBrowserView* view) {
  if (!api_browser_view_ || api_browser_view_->view() != view)
    return;
  DetachBrowserViewImpl();
  api_browser_view_ = nullptr;
}

void NativeWrapperBrowserView::TriggerBeforeunloadEvents() {
  if (!api_browser_view_)
    return;

  auto* vwc = api_browser_view_->view()->web_contents();
  auto* api_web_contents = api::WebContents::From(vwc);

  // Required to make beforeunload handler work.
  if (api_web_contents)
    api_web_contents->NotifyUserActivation();

  if (vwc && vwc->NeedToFireBeforeUnloadOrUnloadEvents())
    vwc->DispatchBeforeUnload(false /* auto_cancel */);
}

void NativeWrapperBrowserView::SetWindowForChildren(NativeWindow* window) {
  if (api_browser_view_)
    api_browser_view_->SetOwnerWindow(window);
}

}  // namespace electron
