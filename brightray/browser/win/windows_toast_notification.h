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
#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/platform_notification_data.h"

using namespace Microsoft::WRL;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Foundation;

class ScopedHString;

namespace WinToasts {

using DesktopToastActivatedEventHandler =
    ITypedEventHandler<ToastNotification*, IInspectable*>;
using DesktopToastDismissedEventHandler =
    ITypedEventHandler<ToastNotification*, ToastDismissedEventArgs*>;

class ToastEventHandler;

class WindowsToastNotification {
 public:
  WindowsToastNotification(
      const char* app_name,
      scoped_ptr<content::DesktopNotificationDelegate> delegate);
  ~WindowsToastNotification();

  void ShowNotification(const WCHAR* title, const WCHAR* msg, std::string iconPath, ComPtr<IToastNotification>& toast);
  void DismissNotification(ComPtr<IToastNotification> toast);
  void NotificationClicked();
  void NotificationDismissed();

 private:
  scoped_ptr<content::DesktopNotificationDelegate> delegate_;
  ComPtr<ToastEventHandler> event_handler_;
  ComPtr<IToastNotificationManagerStatics> toast_manager_;
  ComPtr<IToastNotifier> toast_notifier_;

  bool GetToastXml(IToastNotificationManagerStatics* toastManager, const WCHAR* title, const WCHAR* msg, std::string iconPath, ABI::Windows::Data::Xml::Dom::IXmlDocument** toastXml);
  bool SetXmlText(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc, const WCHAR* text);
  bool SetXmlText(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc, const WCHAR* title, const WCHAR* body);
  bool SetXmlImage(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc, std::string iconPath);
  bool GetTextNodeList(ScopedHString* tag, ABI::Windows::Data::Xml::Dom::IXmlDocument* doc, ABI::Windows::Data::Xml::Dom::IXmlNodeList** nodeList, UINT32 reqLength);
  bool AppendTextToXml(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc, ABI::Windows::Data::Xml::Dom::IXmlNode* node, const WCHAR* text);
  bool SetupCallbacks(IToastNotification* toast);
};


class ToastEventHandler : public RuntimeClass<RuntimeClassFlags<ClassicCom>,
                                              DesktopToastActivatedEventHandler,
                                              DesktopToastDismissedEventHandler> {
 public:
  ToastEventHandler(WindowsToastNotification* notification);
  ~ToastEventHandler();

  IFACEMETHODIMP Invoke(IToastNotification* sender, IInspectable* args);
  IFACEMETHODIMP Invoke(IToastNotification* sender, IToastDismissedEventArgs* e);

 private:
  WindowsToastNotification* notification_;  // weak ref.
};

} // namespace

#endif  // BRIGHTRAY_BROWSER_WIN_WINDOWS_TOAST_NOTIFICATION_H_
