#ifndef BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_
#define BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_

#include <string>

namespace brightray {

class InspectableWebContentsDelegate {
 public:
  virtual ~InspectableWebContentsDelegate() {}

  // Requested by WebContents of devtools.
  virtual void DevToolsSaveToFile(
      const std::string& url, const std::string& content, bool save_as) {}
  virtual void DevToolsAppendToFile(
      const std::string& url, const std::string& content) {}
};

}  // namespace brightray

#endif  // BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_
