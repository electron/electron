#ifndef BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_
#define BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_

#include <string>

namespace brightray {

class InspectableWebContentsDelegate {
 public:
  virtual ~InspectableWebContentsDelegate() {}

  // Called when the devtools is going to change the dock side, returning true
  // to override the default behavior.
  // Receiver should set |succeed| to |false| if it failed to handle this.
  virtual bool DevToolsSetDockSide(const std::string& side, bool* succeed) {
    return false;
  }

  // Called when the devtools is going to be showed, returning true to override
  // the default behavior.
  // Receiver is given the chance to change the |dock_side|.
  virtual bool DevToolsShow(std::string* dock_side) { return false; }

  // Requested by WebContents of devtools.
  virtual void DevToolsSaveToFile(
      const std::string& url, const std::string& content, bool save_as) {}
  virtual void DevToolsAppendToFile(
      const std::string& url, const std::string& content) {}
};

}  // namespace brightray

#endif  // BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_
