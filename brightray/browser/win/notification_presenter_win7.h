#pragma once
#include "brightray/browser/notification_presenter.h"
#include "brightray/browser/win/win32_desktop_notifications/desktop_notification_controller.h"

namespace brightray {

class Win32Notification;

class NotificationPresenterWin7 :
    public NotificationPresenter,
    public DesktopNotificationController {
 public:
    NotificationPresenterWin7() = default;

    Win32Notification* GetNotificationObjectByRef(
        const DesktopNotificationController::Notification& ref);

    Win32Notification* GetNotificationObjectByTag(const std::string& tag);

 private:
    brightray::Notification* CreateNotificationObject(
        NotificationDelegate* delegate) override;

    void OnNotificationClicked(Notification& notification) override;
    void OnNotificationDismissed(Notification& notification) override;

    DISALLOW_COPY_AND_ASSIGN(NotificationPresenterWin7);
};

}   // namespace brightray
