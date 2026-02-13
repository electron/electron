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

#include "base/memory/scoped_refptr.h"
#include "base/task/single_thread_task_runner.h"
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
  void Remove() override;

 private:
  friend class ToastEventHandler;

  HRESULT ShowInternal(const NotificationOptions& options);
  static std::u16string GetToastXml(
      const std::string& notification_id,
      const std::u16string& title,
      const std::u16string& msg,
      const std::wstring& icon_path,
      const std::u16string& timeout_type,
      const bool silent,
      const std::vector<NotificationAction>& actions,
      bool has_reply,
      const std::u16string& reply_placeholder);
  static HRESULT XmlDocumentFromString(
      const wchar_t* xmlString,
      ABI::Windows::Data::Xml::Dom::IXmlDocument** doc);
  HRESULT SetupCallbacks(
      ABI::Windows::UI::Notifications::IToastNotification* toast);
  bool RemoveCallbacks(
      ABI::Windows::UI::Notifications::IToastNotification* toast);

  // Helper methods for async Show() implementation
  static bool CreateToastXmlDocument(
      const NotificationOptions& options,
      NotificationPresenter* presenter,
      const std::string& notification_id,
      base::WeakPtr<Notification> weak_notification,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument>* toast_xml);
  static void CreateToastNotificationOnBackgroundThread(
      const NotificationOptions& options,
      NotificationPresenter* presenter,
      const std::string& notification_id,
      base::WeakPtr<Notification> weak_notification,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner);
  static bool CreateToastNotification(
      ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument> toast_xml,
      const std::string& notification_id,
      base::WeakPtr<Notification> weak_notification,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      ComPtr<ABI::Windows::UI::Notifications::IToastNotification>*
          toast_notification);
  static void SetupAndShowOnUIThread(
      base::WeakPtr<Notification> weak_notification,
      ComPtr<ABI::Windows::UI::Notifications::IToastNotification> notification);
  static void PostNotificationFailedToUIThread(
      base::WeakPtr<Notification> weak_notification,
      const std::string& error,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner);

  static ComPtr<
      ABI::Windows::UI::Notifications::IToastNotificationManagerStatics>*
      toast_manager_;
  static ComPtr<ABI::Windows::UI::Notifications::IToastNotifier>*
      toast_notifier_;

  // Returns the task runner for toast operations, creating it if necessary.
  static scoped_refptr<base::SequencedTaskRunner> GetToastTaskRunner();

  EventRegistrationToken activated_token_;
  EventRegistrationToken dismissed_token_;
  EventRegistrationToken failed_token_;

  ComPtr<ToastEventHandler> event_handler_;
  ComPtr<ABI::Windows::UI::Notifications::IToastNotification>
      toast_notification_;
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

  // DesktopToastActivatedEventHandler
  IFACEMETHODIMP Invoke(
      ABI::Windows::UI::Notifications::IToastNotification* sender,
      IInspectable* args) override;

  // DesktopToastDismissedEventHandler
  IFACEMETHODIMP Invoke(
      ABI::Windows::UI::Notifications::IToastNotification* sender,
      ABI::Windows::UI::Notifications::IToastDismissedEventArgs* e) override;

  // DesktopToastFailedEventHandler
  IFACEMETHODIMP Invoke(
      ABI::Windows::UI::Notifications::IToastNotification* sender,
      ABI::Windows::UI::Notifications::IToastFailedEventArgs* e) override;

 private:
  base::WeakPtr<Notification> notification_;  // weak ref.
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_NOTIFICATION_H_
