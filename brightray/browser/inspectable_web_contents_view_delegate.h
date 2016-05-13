#ifndef BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_
#define BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_

#include "ui/gfx/image/image_skia.h"

namespace brightray {

class InspectableWebContentsViewDelegate {
 public:
  virtual ~InspectableWebContentsViewDelegate() {}

  virtual void DevToolsFocused() {}
  virtual void DevToolsOpened() {}
  virtual void DevToolsClosed() {}

  // Returns the icon of devtools window.
  virtual gfx::ImageSkia GetDevToolsWindowIcon();

#if defined(USE_X11)
  // Called when creating devtools window.
  virtual void GetDevToolsWindowWMClass(
      std::string* name, std::string* class_name) {}
#endif
};

}  // namespace brightray

#endif  // BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_
