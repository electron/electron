#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_H_

namespace content {
struct ShowDesktopNotificationHostMsgParams;
}

namespace brightray {

class NotificationPresenter {
 public:
  NotificationPresenter();
  ~NotificationPresenter();

  void ShowNotification(
      const content::ShowDesktopNotificationHostMsgParams&,
      int render_process_id,
      int render_view_id);
};

}

#endif
