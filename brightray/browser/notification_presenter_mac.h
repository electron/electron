#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_MAC_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_MAC_H_

#import "browser/notification_presenter.h"

#import "base/compiler_specific.h"

namespace brightray {

class NotificationPresenterMac : public NotificationPresenter {
 public:
  NotificationPresenterMac();
  ~NotificationPresenterMac();

  virtual void ShowNotification(
      const content::ShowDesktopNotificationHostMsgParams&,
      int render_process_id,
      int render_view_id) OVERRIDE;
};

}

#endif
