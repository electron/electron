#pragma once
#include "brightray/browser/notification.h"
#include "brightray/browser/win/notification_presenter_win7.h"

namespace brightray {

class Win32Notification : public brightray::Notification {
 public:
    Win32Notification(NotificationDelegate* delegate,
                      NotificationPresenterWin7* presenter) :
        Notification(delegate, presenter) {
    }
    void Show(const base::string16& title, const base::string16& msg,
              const std::string& tag, const GURL& icon_url,
              const SkBitmap& icon, bool silent,
              bool has_reply, const base::string16& reply_placeholder) override;
    void Dismiss() override;

    const DesktopNotificationController::Notification& GetRef() const {
        return notification_ref_;
    }

    const std::string& GetTag() const {
        return tag_;
    }

 private:
    DesktopNotificationController::Notification notification_ref_;
    std::string tag_;

    DISALLOW_COPY_AND_ASSIGN(Win32Notification);
};

}   // namespace brightray
