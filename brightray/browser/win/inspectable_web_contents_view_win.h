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

  // Returns the container control, which has devtools view attached. Unlike
  // GetNativeView(), this returns a views::View instead of HWND, and can only
  // be used by applications that use the views library, if you don't use the
  // views library, you probably want to set dock side to "undocked" before
  // showing the devtools, because devtools is showed attached by default and
  // attached devtools is currently only supported when using views library.
  views::View* GetView() const;

  // Returns the web view control, which can be used by the
  // GetInitiallyFocusedView() to set initial focus to web view.
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

  bool undocked_;
  scoped_ptr<ContainerView> container_;
  base::WeakPtr<DevToolsWindow> devtools_window_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsViewWin);
};

}  // namespace brightray

#endif
