#ifndef BRIGHTRAY_BROWSER_WIN_DEVTOOLS_WINDOW_H_
#define BRIGHTRAY_BROWSER_WIN_DEVTOOLS_WINDOW_H_

#include "base/memory/weak_ptr.h"
#include "ui/gfx/win/window_impl.h"

namespace brightray {

class InspectableWebContentsViewWin;

class DevToolsWindow : public gfx::WindowImpl,
                       public base::SupportsWeakPtr<DevToolsWindow> {
 public:
  static DevToolsWindow* Create(
      InspectableWebContentsViewWin* inspectable_web_contents_view_win);

  CR_BEGIN_MSG_MAP_EX(DevToolsWindow)
    CR_MESSAGE_HANDLER_EX(WM_CREATE, OnCreate)
    CR_MESSAGE_HANDLER_EX(WM_DESTROY, OnDestroy)
    CR_MESSAGE_HANDLER_EX(WM_SIZE, OnSize)
  CR_END_MSG_MAP()

 private:
  explicit DevToolsWindow(
      InspectableWebContentsViewWin* inspectable_web_contents_view_win);
  ~DevToolsWindow();

  LRESULT OnCreate(UINT message, WPARAM, LPARAM);
  LRESULT OnDestroy(UINT message, WPARAM, LPARAM);
  LRESULT OnSize(UINT message, WPARAM, LPARAM);

  InspectableWebContentsViewWin* controller_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsWindow);
};

}  // namespace brightray

#endif
