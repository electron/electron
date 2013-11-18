#ifndef BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_H_
#define BRIGHTRAY_BROWSER_NOTIFICATION_PRESENTER_H_

namespace content {
struct ShowDesktopNotificationHostMsgParams;
}

namespace brightray {

class NotificationPresenter {
 public:
  virtual ~NotificationPresenter() {}

  static NotificationPresenter* Create();

  virtual void ShowNotification(
      const content::ShowDesktopNotificationHostMsgParams&,
      int render_process_id,
      int render_view_id) = 0;
  virtual void CancelNotification(
      int render_process_id,
      int render_view_id,
      int notification_id) = 0;
};

}  // namespace brightray

#endif
