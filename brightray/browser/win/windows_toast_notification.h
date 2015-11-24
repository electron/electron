// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon <jason.poon@microsoft.com>. All rights reserved.
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of this code
// and released it as MIT to the world.

#ifndef BRIGHTRAY_BROWSER_WIN_WINDOWS_TOAST_NOTIFICATION_H_
#define BRIGHTRAY_BROWSER_WIN_WINDOWS_TOAST_NOTIFICATION_H_

#include <windows.h>
#include <windows.ui.notifications.h>
#include <wrl/implements.h>

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/platform_notification_data.h"

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

class WindowsToastNotification {
 public:
  // Should be called before using this class.
  static bool Initialize();

  WindowsToastNotification(
      scoped_ptr<content::DesktopNotificationDelegate> delegate);
  ~WindowsToastNotification();

  void ShowNotification(const std::wstring& title,
                        const std::wstring& msg,
                        std::string icon_path);
  void DismissNotification();

  base::WeakPtr<WindowsToastNotification> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  friend class ToastEventHandler;

  void NotificationClicked();
  void NotificationDismissed();
  void NotificationFailed();

  bool GetToastXml(ABI::Windows::UI::Notifications::IToastNotificationManagerStatics* toastManager,
                   const std::wstring& title,
                   const std::wstring& msg,
                   std::string icon_path,
                   ABI::Windows::Data::Xml::Dom::IXmlDocument** toastXml);
  bool SetXmlText(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                  const std::wstring& text);
  bool SetXmlText(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                  const std::wstring& title,
                  const std::wstring& body);
  bool SetXmlImage(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                   std::string icon_path);
  bool GetTextNodeList(ScopedHString* tag,
                       ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                       ABI::Windows::Data::Xml::Dom::IXmlNodeList** nodeList,
                       UINT32 reqLength);
  bool AppendTextToXml(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                       ABI::Windows::Data::Xml::Dom::IXmlNode* node,
                       const std::wstring& text);
  bool SetupCallbacks(ABI::Windows::UI::Notifications::IToastNotification* toast);

  static ComPtr<ABI::Windows::UI::Notifications::IToastNotificationManagerStatics> toast_manager_;
  static ComPtr<ABI::Windows::UI::Notifications::IToastNotifier> toast_notifier_;

  scoped_ptr<content::DesktopNotificationDelegate> delegate_;
  ComPtr<ToastEventHandler> event_handler_;
  ComPtr<ABI::Windows::UI::Notifications::IToastNotification> toast_notification_;

  base::WeakPtrFactory<WindowsToastNotification> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WindowsToastNotification);
};


class ToastEventHandler : public RuntimeClass<RuntimeClassFlags<ClassicCom>,
                                              DesktopToastActivatedEventHandler,
                                              DesktopToastDismissedEventHandler,
                                              DesktopToastFailedEventHandler> {
 public:
  ToastEventHandler(WindowsToastNotification* notification);
  ~ToastEventHandler();

  IFACEMETHODIMP Invoke(ABI::Windows::UI::Notifications::IToastNotification* sender, IInspectable* args);
  IFACEMETHODIMP Invoke(ABI::Windows::UI::Notifications::IToastNotification* sender, ABI::Windows::UI::Notifications::IToastDismissedEventArgs* e);
  IFACEMETHODIMP Invoke(ABI::Windows::UI::Notifications::IToastNotification* sender, ABI::Windows::UI::Notifications::IToastFailedEventArgs* e);

 private:
  WindowsToastNotification* notification_;  // weak ref.

  DISALLOW_COPY_AND_ASSIGN(ToastEventHandler);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_WIN_WINDOWS_TOAST_NOTIFICATION_H_
