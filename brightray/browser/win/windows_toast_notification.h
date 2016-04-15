// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon <jason.poon@microsoft.com>. All rights reserved.
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of this code
// and released it as MIT to the world.

#ifndef BRIGHTRAY_BROWSER_WIN_WINDOWS_TOAST_NOTIFICATION_H_
#define BRIGHTRAY_BROWSER_WIN_WINDOWS_TOAST_NOTIFICATION_H_

#include <windows.h>
#include <windows.ui.notifications.h>
#include <wrl/implements.h>

#include "browser/notification.h"

using namespace Microsoft::WRL;

class ScopedHString;

namespace brightray {

using DesktopToastActivatedEventHandler =
    ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification*,
    IInspectable*>;
using DesktopToastDismissedEventHandler =
    ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification*,
    ABI::Windows::UI::Notifications::ToastDismissedEventArgs*>;
using DesktopToastFailedEventHandler =
    ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification*,
    ABI::Windows::UI::Notifications::ToastFailedEventArgs*>;

class WindowsToastNotification : public Notification {
 public:
  // Should only be called by NotificationPresenterWin.
  static bool Initialize();

  WindowsToastNotification(NotificationDelegate* delegate,
                           NotificationPresenter* presenter);
  ~WindowsToastNotification();

 protected:
  // Notification:
  void Show(const base::string16& title,
            const base::string16& msg,
            const std::string& tag,
            const GURL& icon_url,
            const SkBitmap& icon,
            const bool silent) override;
  void Dismiss() override;

 private:
  friend class ToastEventHandler;

  bool GetToastXml(ABI::Windows::UI::Notifications::IToastNotificationManagerStatics* toastManager,
                   const std::wstring& title,
                   const std::wstring& msg,
                   const std::wstring& icon_path,
                   const bool silent,
                   ABI::Windows::Data::Xml::Dom::IXmlDocument** toastXml);
  bool SetXmlAudioSilent(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc);
  bool SetXmlText(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                  const std::wstring& text);
  bool SetXmlText(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                  const std::wstring& title,
                  const std::wstring& body);
  bool SetXmlImage(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                   const std::wstring& icon_path);
  bool GetTextNodeList(ScopedHString* tag,
                       ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                       ABI::Windows::Data::Xml::Dom::IXmlNodeList** nodeList,
                       uint32_t reqLength);
  bool AppendTextToXml(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                       ABI::Windows::Data::Xml::Dom::IXmlNode* node,
                       const std::wstring& text);
  bool SetupCallbacks(ABI::Windows::UI::Notifications::IToastNotification* toast);
  bool RemoveCallbacks(ABI::Windows::UI::Notifications::IToastNotification* toast);

  static ComPtr<ABI::Windows::UI::Notifications::IToastNotificationManagerStatics> toast_manager_;
  static ComPtr<ABI::Windows::UI::Notifications::IToastNotifier> toast_notifier_;

  EventRegistrationToken activated_token_;
  EventRegistrationToken dismissed_token_;
  EventRegistrationToken failed_token_;

  ComPtr<ToastEventHandler> event_handler_;
  ComPtr<ABI::Windows::UI::Notifications::IToastNotification> toast_notification_;

  DISALLOW_COPY_AND_ASSIGN(WindowsToastNotification);
};


class ToastEventHandler : public RuntimeClass<RuntimeClassFlags<ClassicCom>,
                                              DesktopToastActivatedEventHandler,
                                              DesktopToastDismissedEventHandler,
                                              DesktopToastFailedEventHandler> {
 public:
  ToastEventHandler(Notification* notification);
  ~ToastEventHandler();

  IFACEMETHODIMP Invoke(ABI::Windows::UI::Notifications::IToastNotification* sender, IInspectable* args);
  IFACEMETHODIMP Invoke(ABI::Windows::UI::Notifications::IToastNotification* sender, ABI::Windows::UI::Notifications::IToastDismissedEventArgs* e);
  IFACEMETHODIMP Invoke(ABI::Windows::UI::Notifications::IToastNotification* sender, ABI::Windows::UI::Notifications::IToastFailedEventArgs* e);

 private:
  base::WeakPtr<Notification> notification_;  // weak ref.

  DISALLOW_COPY_AND_ASSIGN(ToastEventHandler);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_WIN_WINDOWS_TOAST_NOTIFICATION_H_
