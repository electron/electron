// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon
// <jason.poon@microsoft.com>. All rights reserved.
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith
// <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of
// this code
// and released it as MIT to the world.

#include "shell/browser/notifications/win/windows_toast_notification.h"

#include <string_view>

#include <shlobj.h>
#include <wrl\wrappers\corewrappers.h>

#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/string_util_win.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/win/notification_presenter_win.h"
#include "shell/browser/win/scoped_hstring.h"
#include "shell/common/application_info.h"
#include "third_party/libxml/chromium/xml_writer.h"
#include "ui/base/l10n/l10n_util_win.h"
#include "ui/strings/grit/ui_strings.h"

using ABI::Windows::Data::Xml::Dom::IXmlDocument;
using ABI::Windows::Data::Xml::Dom::IXmlDocumentIO;
using Microsoft::WRL::Wrappers::HStringReference;

namespace winui = ABI::Windows::UI;

#define RETURN_IF_FAILED(hr)                           \
  do {                                                 \
    if (const HRESULT _hrTemp = hr; FAILED(_hrTemp)) { \
      return _hrTemp;                                  \
    }                                                  \
  } while (false)

#define REPORT_AND_RETURN_IF_FAILED(hr, msg)                              \
  do {                                                                    \
    if (const HRESULT _hrTemp = hr; FAILED(_hrTemp)) {                    \
      std::string _err =                                                  \
          base::StrCat({msg, ", ERROR ", base::NumberToString(_hrTemp)}); \
      DebugLog(_err);                                                     \
      Notification::NotificationFailed(_err);                             \
      return _hrTemp;                                                     \
    }                                                                     \
  } while (false)

namespace electron {

namespace {

// This string needs to be max 16 characters to work on Windows 10 prior to
// applying Creators Update (build 15063).
constexpr wchar_t kGroup[] = L"Notifications";

void DebugLog(std::string_view log_msg) {
  if (electron::debug_notifications)
    LOG(INFO) << log_msg;
}

std::wstring GetTag(const std::string_view notification_id) {
  return base::NumberToWString(base::FastHash(notification_id));
}

constexpr char kToast[] = "toast";
constexpr char kVisual[] = "visual";
constexpr char kBinding[] = "binding";
constexpr char kTemplate[] = "template";
constexpr char kToastText01[] = "ToastText01";
constexpr char kToastText02[] = "ToastText02";
constexpr char kToastImageAndText01[] = "ToastImageAndText01";
constexpr char kToastImageAndText02[] = "ToastImageAndText02";
constexpr char kText[] = "text";
constexpr char kImage[] = "image";
constexpr char kPlacement[] = "placement";
constexpr char kAppLogoOverride[] = "appLogoOverride";
constexpr char kHintCrop[] = "hint-crop";
constexpr char kHintCropNone[] = "none";
constexpr char kSrc[] = "src";
constexpr char kAudio[] = "audio";
constexpr char kSilent[] = "silent";
constexpr char kTrue[] = "true";
constexpr char kID[] = "id";
constexpr char kScenario[] = "scenario";
constexpr char kReminder[] = "reminder";
constexpr char kActions[] = "actions";
constexpr char kAction[] = "action";
constexpr char kActivationType[] = "activationType";
constexpr char kSystem[] = "system";
constexpr char kArguments[] = "arguments";
constexpr char kDismiss[] = "dismiss";
// The XML version header that has to be stripped from the output.
constexpr char kXmlVersionHeader[] = "<?xml version=\"1.0\"?>\n";

const char* GetTemplateType(bool two_lines, bool has_icon) {
  if (has_icon) {
    return two_lines ? kToastImageAndText02 : kToastImageAndText01;
  }
  return two_lines ? kToastText02 : kToastText01;
}

}  // namespace

// static
ComPtr<winui::Notifications::IToastNotificationManagerStatics>
    WindowsToastNotification::toast_manager_;

// static
ComPtr<winui::Notifications::IToastNotifier>
    WindowsToastNotification::toast_notifier_;

// static
bool WindowsToastNotification::Initialize() {
  // Just initialize, don't care if it fails or already initialized.
  Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);

  ScopedHString toast_manager_str(
      RuntimeClass_Windows_UI_Notifications_ToastNotificationManager);
  if (!toast_manager_str.success())
    return false;
  if (FAILED(Windows::Foundation::GetActivationFactory(toast_manager_str,
                                                       &toast_manager_)))
    return false;

  if (IsRunningInDesktopBridge()) {
    // Ironically, the Desktop Bridge / UWP environment
    // requires us to not give Windows an appUserModelId.
    return SUCCEEDED(toast_manager_->CreateToastNotifier(&toast_notifier_));
  } else {
    ScopedHString app_id;
    if (!GetAppUserModelID(&app_id))
      return false;

    return SUCCEEDED(
        toast_manager_->CreateToastNotifierWithId(app_id, &toast_notifier_));
  }
}

WindowsToastNotification::WindowsToastNotification(
    NotificationDelegate* delegate,
    NotificationPresenter* presenter)
    : Notification(delegate, presenter) {}

WindowsToastNotification::~WindowsToastNotification() {
  // Remove the notification on exit.
  if (toast_notification_) {
    RemoveCallbacks(toast_notification_.Get());
  }
}

void WindowsToastNotification::Show(const NotificationOptions& options) {
  if (SUCCEEDED(ShowInternal(options))) {
    DebugLog("Notification created");

    if (delegate())
      delegate()->NotificationDisplayed();
  }
}

void WindowsToastNotification::Remove() {
  DebugLog("Removing notification from action center");

  ComPtr<winui::Notifications::IToastNotificationManagerStatics2>
      toast_manager2;
  if (FAILED(toast_manager_.As(&toast_manager2)))
    return;

  ComPtr<winui::Notifications::IToastNotificationHistory> notification_history;
  if (FAILED(toast_manager2->get_History(&notification_history)))
    return;

  ScopedHString app_id;
  if (!GetAppUserModelID(&app_id))
    return;

  ScopedHString group(kGroup);
  ScopedHString tag(GetTag(notification_id()));
  notification_history->RemoveGroupedTagWithId(tag, group, app_id);
}

void WindowsToastNotification::Dismiss() {
  DebugLog("Hiding notification");

  toast_notifier_->Hide(toast_notification_.Get());
}

HRESULT WindowsToastNotification::ShowInternal(
    const NotificationOptions& options) {
  ComPtr<IXmlDocument> toast_xml;
  // The custom xml takes priority over the preset template.
  if (!options.toast_xml.empty()) {
    REPORT_AND_RETURN_IF_FAILED(
        XmlDocumentFromString(base::as_wcstr(options.toast_xml), &toast_xml),
        "XML: Invalid XML");
  } else {
    auto* presenter_win = static_cast<NotificationPresenterWin*>(presenter());
    std::wstring icon_path =
        presenter_win->SaveIconToFilesystem(options.icon, options.icon_url);
    std::u16string toast_xml_str =
        GetToastXml(options.title, options.msg, icon_path, options.timeout_type,
                    options.silent);
    REPORT_AND_RETURN_IF_FAILED(
        XmlDocumentFromString(base::as_wcstr(toast_xml_str), &toast_xml),
        "XML: Invalid XML");
  }

  ScopedHString toast_str(
      RuntimeClass_Windows_UI_Notifications_ToastNotification);
  if (!toast_str.success()) {
    NotificationFailed("Creating ScopedHString failed");
    return E_FAIL;
  }

  ComPtr<winui::Notifications::IToastNotificationFactory> toast_factory;
  REPORT_AND_RETURN_IF_FAILED(
      Windows::Foundation::GetActivationFactory(toast_str, &toast_factory),
      "WinAPI: GetActivationFactory failed");

  REPORT_AND_RETURN_IF_FAILED(toast_factory->CreateToastNotification(
                                  toast_xml.Get(), &toast_notification_),
                              "WinAPI: CreateToastNotification failed");

  ComPtr<winui::Notifications::IToastNotification2> toast2;
  REPORT_AND_RETURN_IF_FAILED(
      toast_notification_->QueryInterface(IID_PPV_ARGS(&toast2)),
      "WinAPI: Getting Notification interface failed");

  ScopedHString group(kGroup);
  REPORT_AND_RETURN_IF_FAILED(toast2->put_Group(group),
                              "WinAPI: Setting group failed");

  ScopedHString tag(GetTag(notification_id()));
  REPORT_AND_RETURN_IF_FAILED(toast2->put_Tag(tag),
                              "WinAPI: Setting tag failed");

  REPORT_AND_RETURN_IF_FAILED(SetupCallbacks(toast_notification_.Get()),
                              "WinAPI: SetupCallbacks failed");

  REPORT_AND_RETURN_IF_FAILED(toast_notifier_->Show(toast_notification_.Get()),
                              "WinAPI: Show failed");
  return S_OK;
}

std::u16string WindowsToastNotification::GetToastXml(
    const std::u16string& title,
    const std::u16string& msg,
    const std::wstring& icon_path,
    const std::u16string& timeout_type,
    bool silent) {
  XmlWriter xml_writer;
  xml_writer.StartWriting();

  // <toast ...>
  xml_writer.StartElement(kToast);

  const bool is_reminder = (timeout_type == u"never");
  if (is_reminder) {
    xml_writer.AddAttribute(kScenario, kReminder);
  }

  // <visual>
  xml_writer.StartElement(kVisual);
  // <binding template="<template>">
  xml_writer.StartElement(kBinding);
  const bool two_lines = (!title.empty() && !msg.empty());
  xml_writer.AddAttribute(kTemplate,
                          GetTemplateType(two_lines, !icon_path.empty()));

  // Add text nodes.
  std::u16string line1;
  std::u16string line2;
  if (title.empty() || msg.empty()) {
    line1 = title.empty() ? msg : title;
    if (line1.empty())
      line1 = u"[no message]";
    xml_writer.StartElement(kText);
    xml_writer.AddAttribute(kID, "1");
    xml_writer.AppendElementContent(base::UTF16ToUTF8(line1));
    xml_writer.EndElement();  // </text>
  } else {
    line1 = title;
    line2 = msg;
    xml_writer.StartElement(kText);
    xml_writer.AddAttribute(kID, "1");
    xml_writer.AppendElementContent(base::UTF16ToUTF8(line1));
    xml_writer.EndElement();
    xml_writer.StartElement(kText);
    xml_writer.AddAttribute(kID, "2");
    xml_writer.AppendElementContent(base::UTF16ToUTF8(line2));
    xml_writer.EndElement();
  }

  // Optional icon as app logo override (small icon).
  if (!icon_path.empty()) {
    xml_writer.StartElement(kImage);
    xml_writer.AddAttribute(kPlacement, kAppLogoOverride);
    xml_writer.AddAttribute(kHintCrop, kHintCropNone);
    xml_writer.AddAttribute(kSrc, base::WideToUTF8(icon_path));
    xml_writer.EndElement();  // </image>
  }

  xml_writer.EndElement();  // </binding>
  xml_writer.EndElement();  // </visual>

  // <actions> (only to ensure reminder has a dismiss button).
  if (is_reminder) {
    xml_writer.StartElement(kActions);
    xml_writer.StartElement(kAction);
    xml_writer.AddAttribute(kActivationType, kSystem);
    xml_writer.AddAttribute(kArguments, kDismiss);
    xml_writer.AddAttribute(
        "content", base::WideToUTF8(l10n_util::GetWideString(IDS_APP_CLOSE)));
    xml_writer.EndElement();  // </action>
    xml_writer.EndElement();  // </actions>
  }

  // Silent audio if requested.
  if (silent) {
    xml_writer.StartElement(kAudio);
    xml_writer.AddAttribute(kSilent, kTrue);
    xml_writer.EndElement();  // </audio>
  }

  xml_writer.EndElement();  // </toast>

  xml_writer.StopWriting();
  std::string xml = xml_writer.GetWrittenString();
  if (base::StartsWith(xml, kXmlVersionHeader, base::CompareCase::SENSITIVE)) {
    xml.erase(0, sizeof(kXmlVersionHeader) - 1);
  }

  return base::UTF8ToUTF16(xml);
}

HRESULT WindowsToastNotification::XmlDocumentFromString(
    const wchar_t* xmlString,
    IXmlDocument** doc) {
  ComPtr<IXmlDocument> xmlDoc;
  RETURN_IF_FAILED(Windows::Foundation::ActivateInstance(
      HStringReference(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument).Get(),
      &xmlDoc));

  ComPtr<IXmlDocumentIO> docIO;
  RETURN_IF_FAILED(xmlDoc.As(&docIO));

  RETURN_IF_FAILED(docIO->LoadXml(HStringReference(xmlString).Get()));

  return xmlDoc.CopyTo(doc);
}

HRESULT WindowsToastNotification::SetupCallbacks(
    winui::Notifications::IToastNotification* toast) {
  event_handler_ = Make<ToastEventHandler>(this);
  RETURN_IF_FAILED(
      toast->add_Activated(event_handler_.Get(), &activated_token_));
  RETURN_IF_FAILED(
      toast->add_Dismissed(event_handler_.Get(), &dismissed_token_));
  RETURN_IF_FAILED(toast->add_Failed(event_handler_.Get(), &failed_token_));
  return S_OK;
}

bool WindowsToastNotification::RemoveCallbacks(
    winui::Notifications::IToastNotification* toast) {
  if (FAILED(toast->remove_Activated(activated_token_)))
    return false;

  if (FAILED(toast->remove_Dismissed(dismissed_token_)))
    return false;

  return SUCCEEDED(toast->remove_Failed(failed_token_));
}

/*
/ Toast Event Handler
*/
ToastEventHandler::ToastEventHandler(Notification* notification)
    : notification_(notification->GetWeakPtr()) {}

ToastEventHandler::~ToastEventHandler() = default;

IFACEMETHODIMP ToastEventHandler::Invoke(
    winui::Notifications::IToastNotification* sender,
    IInspectable* args) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&Notification::NotificationClicked, notification_));
  DebugLog("Notification clicked");

  return S_OK;
}

IFACEMETHODIMP ToastEventHandler::Invoke(
    winui::Notifications::IToastNotification* sender,
    winui::Notifications::IToastDismissedEventArgs* e) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&Notification::NotificationDismissed,
                                notification_, false));
  DebugLog("Notification dismissed");
  return S_OK;
}

IFACEMETHODIMP ToastEventHandler::Invoke(
    winui::Notifications::IToastNotification* sender,
    winui::Notifications::IToastFailedEventArgs* e) {
  HRESULT error;
  e->get_ErrorCode(&error);
  std::string errorMessage = base::StrCat(
      {"Notification failed. HRESULT:", base::NumberToString(error)});
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&Notification::NotificationFailed,
                                notification_, errorMessage));
  DebugLog(errorMessage);

  return S_OK;
}

}  // namespace electron
