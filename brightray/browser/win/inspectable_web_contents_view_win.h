#ifndef BRIGHTRAY_BROWSER_WIN_INSPECTABLE_WEB_CONTENTS_VIEW_WIN_H_
#define BRIGHTRAY_BROWSER_WIN_INSPECTABLE_WEB_CONTENTS_VIEW_WIN_H_

#include "browser/inspectable_web_contents_view.h"

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"

namespace views {
class View;
}

namespace brightray {

class ContainerView;
class DevToolsWindow;
class InspectableWebContentsImpl;

class InspectableWebContentsViewWin : public InspectableWebContentsView {
 public:
  explicit InspectableWebContentsViewWin(
      InspectableWebContentsImpl* inspectable_web_contents_impl);
  ~InspectableWebContentsViewWin();

  views::View* GetView() const;
  views::View* GetWebView() const;

  virtual gfx::NativeView GetNativeView() const OVERRIDE;
  virtual void ShowDevTools() OVERRIDE;
  virtual void CloseDevTools() OVERRIDE;
  virtual bool IsDevToolsViewShowing() OVERRIDE;
  virtual bool SetDockSide(const std::string& side) OVERRIDE;

  InspectableWebContentsImpl* inspectable_web_contents() {
    return inspectable_web_contents_;
  }

 private:
  // Owns us.
  InspectableWebContentsImpl* inspectable_web_contents_;

  scoped_ptr<ContainerView> container_;

  base::WeakPtr<DevToolsWindow> devtools_window_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsViewWin);
};

}  // namespace brightray

#endif
