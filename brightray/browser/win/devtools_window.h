#ifndef BRIGHTRAY_BROWSER_WIN_DEVTOOLS_WINDOW_H_
#define BRIGHTRAY_BROWSER_WIN_DEVTOOLS_WINDOW_H_

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"

namespace views {
class Widget;
}

namespace brightray {

class InspectableWebContentsViewWin;

class DevToolsWindow : public base::SupportsWeakPtr<DevToolsWindow> {
 public:
  static DevToolsWindow* Create(
      InspectableWebContentsViewWin* inspectable_web_contents_view_win);

  void Show();
  void Close();
  void Destroy();

 private:
  explicit DevToolsWindow(
      InspectableWebContentsViewWin* inspectable_web_contents_view_win);
  ~DevToolsWindow();

  InspectableWebContentsViewWin* controller_;
  scoped_ptr<views::Widget> widget_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsWindow);
};

}  // namespace brightray

#endif
