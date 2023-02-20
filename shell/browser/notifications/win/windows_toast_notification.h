// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon
// <jason.poon@microsoft.com>. All rights reserved.
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith
// <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of
// this code
// and released it as MIT to the world.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_NOTIFICATION_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_NOTIFICATION_H_

#include <windows.h>
#include <windows.ui.notifications.h>
#include <wrl/implements.h>
#include <string>
#include <vector>

#include "shell/browser/notifications/notification.h"

using Microsoft::WRL::ClassicCom;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Make;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;

namespace electron {

class ScopedHString;

using DesktopToastActivatedEventHandler =
    ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification*,
        IInspectable*>;
using DesktopToastDismissedEventHandler =
    ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification*,
        ABI::Windows::UI::Notifications::ToastDismissedEventArgs*>;
using DesktopToastFailedEventHandler =
    ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification*,
        ABI::Windows::UI::Notifications::ToastFailedEventArgs*>;

class WindowsToastNotification : public Notification {
 public:
  // Should only be called by NotificationPresenterWin.
  static bool Initialize();

  WindowsToastNotification(NotificationDelegate* delegate,
                           NotificationPresenter* presenter);
  ~WindowsToastNotification() override;

 protected:
  // Notification:
  void Show(const NotificationOptions& options) override;
  void Dismiss() override;

 private:
  friend class ToastEventHandler;

  HRESULT ShowInternal(const NotificationOptions& options);
  HRESULT SetToastXml(
      ABI::Windows::Data::Xml::Dom::IXmlDocument* toast_xml,
      const std::wstring& title,
      const std::wstring& msg,
      const std::wstring& icon_path,
      const std::wstring& image_path,
      const std::wstring& timeout_type,
      const std::wstring& data,
      const std::vector<electron::NotificationAction>& actions_list,
      const bool silent,
      const bool require_interaction);
  HRESULT SetXmlAudioSilent(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc);
  HRESULT SetXmlScenarioReminder(
      ABI::Windows::Data::Xml::Dom::IXmlDocument* doc);
  HRESULT SetXmlScenarioType(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                             const std::wstring& scenario_type);
  HRESULT SetXmlText(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                     const std::wstring& text);
  HRESULT SetXmlImage(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                      const std::wstring& image_path,
                      const std::wstring& placement);
  HRESULT SetLaunchParams(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                          const std::wstring& params);
  HRESULT AppendTextToXml(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                          ABI::Windows::Data::Xml::Dom::IXmlNode* node,
                          const std::wstring& text);
  HRESULT AddAction(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                    const std::wstring& type,
                    const std::wstring& content,  // equal to id for input
                    const std::wstring& arguments,
                    const std::wstring& icon_path,
                    const std::wstring& placeholder,
                    const std::wstring& hint_inputId);
  HRESULT XmlDocumentFromString(
      const wchar_t* xmlString,
      ABI::Windows::Data::Xml::Dom::IXmlDocument** doc);
  HRESULT SetupCallbacks(
      ABI::Windows::UI::Notifications::IToastNotification* toast);
  HRESULT RemoveCallbacks(
      ABI::Windows::UI::Notifications::IToastNotification* toast);

  static ComPtr<
      ABI::Windows::UI::Notifications::IToastNotificationManagerStatics>
      toast_manager_;
  static ComPtr<ABI::Windows::UI::Notifications::IToastNotifier>
      toast_notifier_;

  EventRegistrationToken activated_token_;
  EventRegistrationToken dismissed_token_;
  EventRegistrationToken failed_token_;

  ComPtr<ToastEventHandler> event_handler_;
  ComPtr<ABI::Windows::UI::Notifications::IToastNotification>
      toast_notification_;
  PCWSTR AsString(
      const ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument>& xmlDocument);
};

class ToastEventHandler : public RuntimeClass<RuntimeClassFlags<ClassicCom>,
                                              DesktopToastActivatedEventHandler,
                                              DesktopToastDismissedEventHandler,
                                              DesktopToastFailedEventHandler> {
 public:
  explicit ToastEventHandler(Notification* notification);
  ~ToastEventHandler() override;

  // disable copy
  ToastEventHandler(const ToastEventHandler&) = delete;
  ToastEventHandler& operator=(const ToastEventHandler&) = delete;

  IFACEMETHODIMP Invoke(
      ABI::Windows::UI::Notifications::IToastNotification* sender,
      IInspectable* args) override;
  IFACEMETHODIMP Invoke(
      ABI::Windows::UI::Notifications::IToastNotification* sender,
      ABI::Windows::UI::Notifications::IToastDismissedEventArgs* e) override;
  IFACEMETHODIMP Invoke(
      ABI::Windows::UI::Notifications::IToastNotification* sender,
      ABI::Windows::UI::Notifications::IToastFailedEventArgs* e) override;
  // feat: Upgrade for persistent notifications support
  void SetNotificationOptions(const NotificationOptions& options) {
    options_ = options;
  }

 private:
  // feat: Clear notifications
  friend class WindowsToastNotification;
  static ComPtr<
      ABI::Windows::UI::Notifications::IToastNotificationManagerStatics2>
      toast_manager_;
  // feat: Upgrade for persistent notifications support
  NotificationOptions options_;
  base::WeakPtr<Notification> notification_;  // weak ref.
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_NOTIFICATION_H_
