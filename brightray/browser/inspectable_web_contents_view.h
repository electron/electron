#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_H_

#include "ui/gfx/native_widget_types.h"

namespace brightray {

class InspectableWebContentsView {
 public:
  virtual ~InspectableWebContentsView() {}

  virtual gfx::NativeView GetNativeView() const = 0;

  virtual void ShowDevTools() = 0;
  virtual void CloseDevTools() = 0;
  virtual bool SetDockSide(const std::string& side) = 0;
};

}  // namespace brightray

#endif
