#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_H_

#include "base/callback_forward.h"
#include "base/memory/scoped_ptr.h"

class SkBitmap;

namespace content {
class DesktopNotificationDelegate;
struct PlatformNotificationData;
}

namespace brightray {

class NotificationPresenter {
 public:
  virtual ~NotificationPresenter() {}

  static NotificationPresenter* Create();

  virtual void ShowNotification(
      const content::PlatformNotificationData&,
      const SkBitmap& icon,
      scoped_ptr<content::DesktopNotificationDelegate> delegate,
      base::Closure* cancel_callback) = 0;
};

}  // namespace brightray

#endif
