#ifndef BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_
#define BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_

#include <string>

#include "ui/gfx/image/image_skia.h"

namespace brightray {

class InspectableWebContentsDelegate {
 public:
  virtual ~InspectableWebContentsDelegate() {}

  // Returns the icon of devtools window.
  virtual gfx::ImageSkia GetDevToolsWindowIcon();

  // Requested by WebContents of devtools.
  virtual void DevToolsSaveToFile(
      const std::string& url, const std::string& content, bool save_as) {}
  virtual void DevToolsAppendToFile(
      const std::string& url, const std::string& content) {}
  virtual void DevToolsFocused() {}
  virtual void DevToolsAddFileSystem() {}
  virtual void DevToolsRemoveFileSystem(
      const std::string& file_system_path) {}
  virtual void DevToolsOpened() {}
  virtual void DevToolsClosed() {}

#if defined(USE_X11)
  // Called when creating devtools window.
  virtual void GetDevToolsWindowWMClass(
      std::string* name, std::string* class_name) {}
#endif
};

}  // namespace brightray

#endif  // BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_
