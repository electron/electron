#include "brightray/browser/win/notification_presenter_win7.h"

#include <string>

#include "brightray/browser/win/win32_notification.h"

namespace brightray {

brightray::Notification* NotificationPresenterWin7::CreateNotificationObject(
    NotificationDelegate* delegate) {
    return new Win32Notification(delegate, this);
}

Win32Notification* NotificationPresenterWin7::GetNotificationObjectByRef(
    const DesktopNotificationController::Notification& ref) {
    for (auto n : this->notifications()) {
        auto w32n = static_cast<Win32Notification*>(n);
        if (w32n->GetRef() == ref)
            return w32n;
    }

    return nullptr;
}

Win32Notification* NotificationPresenterWin7::GetNotificationObjectByTag(
    const std::string& tag) {
    for (auto n : this->notifications()) {
        auto w32n = static_cast<Win32Notification*>(n);
        if (w32n->GetTag() == tag)
            return w32n;
    }

    return nullptr;
}

void NotificationPresenterWin7::OnNotificationClicked(
    Notification& notification) {
    auto n = GetNotificationObjectByRef(notification);
    if (n) n->NotificationClicked();
}

void NotificationPresenterWin7::OnNotificationDismissed(
    Notification& notification) {
    auto n = GetNotificationObjectByRef(notification);
    if (n) n->NotificationDismissed();
}

}   // namespace brightray
