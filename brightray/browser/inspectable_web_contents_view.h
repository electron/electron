#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_H_

#include "ui/gfx/native_widget_types.h"

class DevToolsContentsResizingStrategy;

#if defined(TOOLKIT_VIEWS)
namespace views {
class View;
}
#endif

namespace brightray {

class InspectableWebContentsView {
 public:
  virtual ~InspectableWebContentsView() {}

#if defined(TOOLKIT_VIEWS)
  // Returns the container control, which has devtools view attached.
  virtual views::View* GetView() = 0;

  // Returns the web view control, which can be used by the
  // GetInitiallyFocusedView() to set initial focus to web view.
  virtual views::View* GetWebView() = 0;
#else
  virtual gfx::NativeView GetNativeView() const = 0;
#endif

  virtual void ShowDevTools() = 0;
  // Hide the DevTools view.
  virtual void CloseDevTools() = 0;
  virtual bool IsDevToolsViewShowing() = 0;
  virtual void SetIsDocked(bool docked) = 0;
  virtual void SetContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy) = 0;
};

}  // namespace brightray

#endif
