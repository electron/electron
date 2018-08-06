#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_H_

#include <string>

#include "content/public/browser/web_contents.h"

namespace base {
class Value;
}

namespace content {
class DevToolsAgentHost;
}

namespace brightray {

class InspectableWebContentsDelegate;
class InspectableWebContentsView;

class InspectableWebContents {
 public:
  // The returned InspectableWebContents takes ownership of the passed-in
  // WebContents.
  static InspectableWebContents* Create(content::WebContents* web_contents,
                                        bool is_guest);

  virtual ~InspectableWebContents() {}

  virtual InspectableWebContentsView* GetView() const = 0;
  virtual content::WebContents* GetWebContents() const = 0;
  virtual content::WebContents* GetDevToolsWebContents() const = 0;

  // The delegate manages its own life.
  virtual void SetDelegate(InspectableWebContentsDelegate* delegate) = 0;
  virtual InspectableWebContentsDelegate* GetDelegate() const = 0;

  virtual bool IsGuest() const = 0;
  virtual void SetDevToolsWebContents(content::WebContents* devtools) = 0;
  virtual void SetDockState(const std::string& state) = 0;
  virtual void ShowDevTools() = 0;
  virtual void CloseDevTools() = 0;
  virtual bool IsDevToolsViewShowing() = 0;
  virtual void AttachTo(scoped_refptr<content::DevToolsAgentHost>) = 0;
  virtual void Detach() = 0;
  virtual void CallClientFunction(const std::string& function_name,
                                  const base::Value* arg1 = nullptr,
                                  const base::Value* arg2 = nullptr,
                                  const base::Value* arg3 = nullptr) = 0;
  virtual void InspectElement(int x, int y) = 0;
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_H_
