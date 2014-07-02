#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_H_

#include "ui/gfx/native_widget_types.h"

class DevToolsContentsResizingStrategy;

namespace brightray {

class InspectableWebContentsView {
 public:
  virtual ~InspectableWebContentsView() {}

  virtual gfx::NativeView GetNativeView() const = 0;

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
