#ifndef SHELL_BROWSER_UI_NATIVE_WRAPPER_BROWSER_VIEW_H_
#define SHELL_BROWSER_UI_NATIVE_WRAPPER_BROWSER_VIEW_H_

#include "shell/browser/ui/native_view.h"

namespace electron {

class NativeBrowserView;

namespace api {
class BrowserView;
}

class NativeWrapperBrowserView : public NativeView {
 public:
  NativeWrapperBrowserView();

  void SetBrowserView(api::BrowserView* browser_view);
  void DetachBrowserView(NativeBrowserView* view);

  // NativeView:
  void SetBounds(const gfx::Rect& bounds) override;
  void TriggerBeforeunloadEvents() override;
#if defined(OS_MAC)
  void UpdateDraggableRegions() override;
#endif

 protected:
  ~NativeWrapperBrowserView() override;

  // NativeView:
  void SetWindowForChildren(NativeWindow* window) override;

  void InitWrapperBrowserView();
  void SetBrowserViewImpl();
  void DetachBrowserViewImpl();

 private:
  api::BrowserView* api_browser_view_ = nullptr;
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_NATIVE_WRAPPER_BROWSER_VIEW_H_
