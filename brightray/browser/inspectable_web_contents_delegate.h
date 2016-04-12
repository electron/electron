#ifndef BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_
#define BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_

#include <string>

namespace brightray {

class InspectableWebContentsDelegate {
 public:
  virtual ~InspectableWebContentsDelegate() {}

  // Requested by WebContents of devtools.
  virtual void DevToolsReloadPage() {}
  virtual void DevToolsSaveToFile(
      const std::string& url, const std::string& content, bool save_as) {}
  virtual void DevToolsAppendToFile(
      const std::string& url, const std::string& content) {}
  virtual void DevToolsRequestFileSystems() {}
  virtual void DevToolsAddFileSystem(
      const base::FilePath& file_system_path) {}
  virtual void DevToolsRemoveFileSystem(
      const base::FilePath& file_system_path) {}
};

}  // namespace brightray

#endif  // BRIGHTRAY_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_
