#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_H_

#include "base/callback_forward.h"

namespace content {
class DesktopNotificationDelegate;
struct ShowDesktopNotificationHostMsgParams;
}

namespace brightray {

class NotificationPresenter {
 public:
  virtual ~NotificationPresenter() {}

  static NotificationPresenter* Create();

  virtual void ShowNotification(
      const content::ShowDesktopNotificationHostMsgParams&,
      content::DesktopNotificationDelegate* delegate,
      base::Closure* cancel_callback) = 0;
};

}  // namespace brightray

#endif
