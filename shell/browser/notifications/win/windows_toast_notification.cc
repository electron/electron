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

#include "base/containers/fixed_flat_map.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/string_util_win.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
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

#define REPORT_AND_RETURN_IF_FAILED(hr, msg)                               \
  do {                                                                     \
    if (const HRESULT _hrTemp = hr; FAILED(_hrTemp)) {                     \
      std::string _err =                                                   \
          base::StrCat({msg, ", ERROR ", FailureResultToString(_hrTemp)}); \
      DebugLog(_err);                                                      \
      Notification::NotificationFailed(_err);                              \
      return _hrTemp;                                                      \
    }                                                                      \
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

// See https://www.hresult.info for HRESULT error codes.
const std::string FailureResultToString(HRESULT failure_reason) {
  static constexpr auto kFailureMessages = base::MakeFixedFlatMap<
      HRESULT, std::string_view>(
      {{-2143420143, "Settings prevent the notification from being delivered."},
       {-2143420142,
        "Application capabilities prevent the notification from being "
        "delivered."},
       {-2143420140,
        "Settings prevent the notification type from being delivered."},
       {-2143420139, "The size of the notification content is too large."},
       {-2143420138, "The size of the notification tag is too large."},
       {-2143420155, "The notification platform is unavailable."},
       {-2143420154, "The notification has already been posted."},
       {-2143420153, "The notification has already been hidden."},
       {-2143420128,
        "The size of the developer id for scheduled notification is too "
        "large."},
       {-2143420118, "The notification tag is not alphanumeric."},
       {-2143419897,
        "Toast Notification was dropped without being displayed to the user."},
       {-2143419896,
        "The notification platform does not have the proper privileges to "
        "complete the request."}});

  std::string hresult_str = base::StrCat(
      {" (HRESULT: ", base::NumberToString(static_cast<long>(failure_reason)),
       ")"});

  if (const auto it = kFailureMessages.find(failure_reason);
      it != kFailureMessages.end()) {
    return base::StrCat({"Notification failed - ", it->second, hresult_str});
  }

  return hresult_str;
}

constexpr char kPlaceholderContent[] = "placeHolderContent";
constexpr char kContent[] = "content";
constexpr char kToast[] = "toast";
constexpr char kVisual[] = "visual";
constexpr char kBinding[] = "binding";
constexpr char kTemplate[] = "template";
constexpr char kToastTemplate[] = "ToastGeneric";
constexpr char kText[] = "text";
constexpr char kImage[] = "image";
constexpr char kPlacement[] = "placement";
constexpr char kAppLogoOverride[] = "appLogoOverride";
constexpr char kHintCrop[] = "hint-crop";
constexpr char kHintInputId[] = "hint-inputId";
constexpr char kHintCropNone[] = "none";
constexpr char kSrc[] = "src";
constexpr char kAudio[] = "audio";
constexpr char kSilent[] = "silent";
constexpr char kReply[] = "reply";
constexpr char kTrue[] = "true";
constexpr char kID[] = "id";
constexpr char kInput[] = "input";
constexpr char kType[] = "type";
constexpr char kSelection[] = "selection";
constexpr char kScenario[] = "scenario";
constexpr char kReminder[] = "reminder";
constexpr char kActions[] = "actions";
constexpr char kAction[] = "action";
constexpr char kActivationType[] = "activationType";
constexpr char kActivationTypeForeground[] = "foreground";
constexpr char kActivationTypeSystem[] = "system";
constexpr char kArguments[] = "arguments";
constexpr char kDismiss[] = "dismiss";
// The XML version header that has to be stripped from the output.
constexpr char kXmlVersionHeader[] = "<?xml version=\"1.0\"?>\n";

}  // namespace

// static
ComPtr<winui::Notifications::IToastNotificationManagerStatics>*
    WindowsToastNotification::toast_manager_ = nullptr;

// static
ComPtr<winui::Notifications::IToastNotifier>*
    WindowsToastNotification::toast_notifier_ = nullptr;

// static
scoped_refptr<base::SequencedTaskRunner>
WindowsToastNotification::GetToastTaskRunner() {
  // Use function-local static to avoid exit-time destructor warning
  static base::NoDestructor<scoped_refptr<base::SequencedTaskRunner>>
      task_runner(base::ThreadPool::CreateSequencedTaskRunner(
          {base::TaskPriority::USER_BLOCKING,
           base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN,
           base::MayBlock()}));
  return *task_runner;
}

bool WindowsToastNotification::Initialize() {
  HRESULT hr = Windows::Foundation::Initialize(RO_INIT_SINGLETHREADED);
  if (FAILED(hr))
    Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);

  ScopedHString toast_manager_str(
      RuntimeClass_Windows_UI_Notifications_ToastNotificationManager);
  if (!toast_manager_str.success())
    return false;

  if (!toast_manager_) {
    toast_manager_ = new ComPtr<
        ABI::Windows::UI::Notifications::IToastNotificationManagerStatics>();
  }

  if (FAILED(Windows::Foundation::GetActivationFactory(
          toast_manager_str, toast_manager_->GetAddressOf())))
    return false;

  if (!toast_notifier_) {
    toast_notifier_ =
        new ComPtr<ABI::Windows::UI::Notifications::IToastNotifier>();
  }

  if (IsRunningInDesktopBridge()) {
    // Ironically, the Desktop Bridge / UWP environment
    // requires us to not give Windows an appUserModelId.
    return SUCCEEDED(
        (*toast_manager_)
            ->CreateToastNotifier(toast_notifier_->GetAddressOf()));
  } else {
    ScopedHString app_id;
    if (!GetAppUserModelID(&app_id))
      return false;

    return SUCCEEDED((*toast_manager_)
                         ->CreateToastNotifierWithId(
                             app_id, toast_notifier_->GetAddressOf()));
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

// This method posts a request onto the toast background thread, which
// creates the toast xml then posts notification creation to the UI thread. This
// avoids blocking the UI for expensive XML parsing and COM initialization or
// the COM server becoming unavailable. All needed fields are captured before
// posting the task.
// The method will eventually result in a display or failure signal being posted
// back to the UI thread, where further callbacks (clicked, dismissed, failed)
// are handled by ToastEventHandler.
void WindowsToastNotification::Show(const NotificationOptions& options) {
  DebugLog("WindowsToastNotification::Show called");
  DebugLog(base::StrCat(
      {"toast_xml empty: ", options.toast_xml.empty() ? "yes" : "no"}));
  if (!options.toast_xml.empty()) {
    DebugLog(base::StrCat({"toast_xml length: ",
                           base::NumberToString(options.toast_xml.length())}));
  }

  // Capture all needed data on UI thread before posting to background thread
  std::string notif_id = notification_id();
  NotificationPresenter* presenter_ptr = presenter();
  base::WeakPtr<Notification> weak_this = GetWeakPtr();
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner =
      content::GetUIThreadTaskRunner({});

  DebugLog("Posting task to background thread");
  auto task_runner = GetToastTaskRunner();
  DebugLog(base::StrCat({"Task runner valid: ", task_runner ? "yes" : "no"}));

  // Post Show operation to background thread to prevent blocking
  // This is the main entry point for the notification creation process
  bool posted = task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(
          &WindowsToastNotification::CreateToastNotificationOnBackgroundThread,
          options, presenter_ptr, notif_id, weak_this, ui_task_runner));
  DebugLog(base::StrCat(
      {"Task posted to background thread: ", posted ? "yes" : "no"}));
}

// Creates the toast XML on the background thread. If the XML is invalid, posts
// a failure event back to the UI thread. Otherwise, continues to create the
// toast notification on the background thread.
void WindowsToastNotification::CreateToastNotificationOnBackgroundThread(
    const NotificationOptions& options,
    NotificationPresenter* presenter,
    const std::string& notification_id,
    base::WeakPtr<Notification> weak_notification,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner) {
  DebugLog("CreateToastXmlOnBackgroundThread called");
  ComPtr<IXmlDocument> toast_xml;

  if (!CreateToastXmlDocument(options, presenter, notification_id,
                              weak_notification, ui_task_runner, &toast_xml)) {
    return;  // Error already posted to UI thread
  }

  // Continue to create the toast notification
  ComPtr<ABI::Windows::UI::Notifications::IToastNotification>
      toast_notification;
  if (!CreateToastNotification(toast_xml, notification_id, weak_notification,
                               ui_task_runner, &toast_notification)) {
    return;  // Error already posted to UI thread
  }

  // Setup callbacks and show on UI thread (Show must be called on UI thread)
  scoped_refptr<base::SingleThreadTaskRunner> ui_runner =
      content::GetUIThreadTaskRunner({});
  ui_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&WindowsToastNotification::SetupAndShowOnUIThread,
                     weak_notification, toast_notification));
}

// Creates the toast XML document on the background thread. Returns true on
// success, false on failure. On failure, posts error to UI thread. static
bool WindowsToastNotification::CreateToastXmlDocument(
    const NotificationOptions& options,
    NotificationPresenter* presenter,
    const std::string& notification_id,
    base::WeakPtr<Notification> weak_notification,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    ComPtr<IXmlDocument>* toast_xml) {
  // The custom xml takes priority over the preset template.
  if (!options.toast_xml.empty()) {
    DebugLog("Toast XML (custom) id=" + notification_id + ": " +
             base::UTF16ToUTF8(options.toast_xml));
    HRESULT hr = XmlDocumentFromString(base::as_wcstr(options.toast_xml),
                                       toast_xml->GetAddressOf());
    DebugLog(base::StrCat({"XmlDocumentFromString returned HRESULT: ",
                           base::NumberToString(hr)}));
    if (FAILED(hr)) {
      std::string err =
          base::StrCat({"XML: Invalid XML, ERROR ", FailureResultToString(hr)});
      DebugLog(base::StrCat({"XML parsing failed, posting error: ", err}));
      PostNotificationFailedToUIThread(weak_notification, err, ui_task_runner);
      return false;
    }
    DebugLog("XML parsing succeeded");
  } else {
    auto* presenter_win = static_cast<NotificationPresenterWin*>(presenter);
    std::wstring icon_path =
        presenter_win->SaveIconToFilesystem(options.icon, options.icon_url);
    std::u16string toast_xml_str =
        GetToastXml(notification_id, options.title, options.msg, icon_path,
                    options.timeout_type, options.silent, options.actions,
                    options.has_reply, options.reply_placeholder);

    DebugLog("Toast XML (generated) id=" + notification_id + ": " +
             base::UTF16ToUTF8(toast_xml_str));

    HRESULT hr = XmlDocumentFromString(base::as_wcstr(toast_xml_str),
                                       toast_xml->GetAddressOf());
    if (FAILED(hr)) {
      std::string err =
          base::StrCat({"XML: Invalid XML, ERROR ", FailureResultToString(hr)});
      DebugLog(err);
      PostNotificationFailedToUIThread(weak_notification, err, ui_task_runner);
      return false;
    }
  }
  return true;
}

// Creates the toast notification on the background thread. Returns true on
// success, false on failure. On failure, posts error to UI thread. On success,
// returns the created notification via out parameter.
bool WindowsToastNotification::CreateToastNotification(
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument> toast_xml,
    const std::string& notification_id,
    base::WeakPtr<Notification> weak_notification,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    ComPtr<ABI::Windows::UI::Notifications::IToastNotification>*
        toast_notification) {
  ScopedHString toast_str(
      RuntimeClass_Windows_UI_Notifications_ToastNotification);
  if (!toast_str.success()) {
    PostNotificationFailedToUIThread(
        weak_notification, "Creating ScopedHString failed", ui_task_runner);
    return false;
  }

  ComPtr<winui::Notifications::IToastNotificationFactory> toast_factory;
  HRESULT hr =
      Windows::Foundation::GetActivationFactory(toast_str, &toast_factory);
  if (FAILED(hr)) {
    std::string err =
        base::StrCat({"WinAPI: GetActivationFactory failed, ERROR ",
                      FailureResultToString(hr)});
    DebugLog(err);
    PostNotificationFailedToUIThread(weak_notification, err, ui_task_runner);
    return false;
  }

  hr = toast_factory->CreateToastNotification(
      toast_xml.Get(), toast_notification->GetAddressOf());
  if (FAILED(hr)) {
    std::string err =
        base::StrCat({"WinAPI: CreateToastNotification failed, ERROR ",
                      FailureResultToString(hr)});
    DebugLog(err);
    PostNotificationFailedToUIThread(weak_notification, err, ui_task_runner);
    return false;
  }

  ComPtr<winui::Notifications::IToastNotification2> toast2;
  hr = (*toast_notification)->QueryInterface(IID_PPV_ARGS(&toast2));
  if (FAILED(hr)) {
    std::string err =
        base::StrCat({"WinAPI: Getting Notification interface failed, ERROR ",
                      FailureResultToString(hr)});
    DebugLog(err);
    PostNotificationFailedToUIThread(weak_notification, err, ui_task_runner);
    return false;
  }

  ScopedHString group(kGroup);
  hr = toast2->put_Group(group);
  if (FAILED(hr)) {
    std::string err = base::StrCat(
        {"WinAPI: Setting group failed, ERROR ", FailureResultToString(hr)});
    DebugLog(err);
    PostNotificationFailedToUIThread(weak_notification, err, ui_task_runner);
    return false;
  }

  ScopedHString tag(GetTag(notification_id));
  hr = toast2->put_Tag(tag);
  if (FAILED(hr)) {
    std::string err = base::StrCat(
        {"WinAPI: Setting tag failed, ERROR ", FailureResultToString(hr)});
    DebugLog(err);
    PostNotificationFailedToUIThread(weak_notification, err, ui_task_runner);
    return false;
  }

  return true;
}

// Sets up callbacks and shows the notification on the UI thread.
// This part has to be called on the UI thread. This WinRT API
// does not allow calls from background threads.
void WindowsToastNotification::SetupAndShowOnUIThread(
    base::WeakPtr<Notification> weak_notification,
    ComPtr<ABI::Windows::UI::Notifications::IToastNotification> notification) {
  auto* notif = static_cast<WindowsToastNotification*>(weak_notification.get());
  if (!notif)
    return;

  // Setup callbacks and store notification on UI thread
  HRESULT hr = notif->SetupCallbacks(notification.Get());
  if (FAILED(hr)) {
    std::string err = base::StrCat(
        {"WinAPI: SetupCallbacks failed, ERROR ", FailureResultToString(hr)});
    DebugLog(err);
    notif->NotificationFailed(err);
    return;
  }

  notif->toast_notification_ = notification;

  // Show notification on UI thread (must be called on UI thread)
  hr = (*toast_notifier_)->Show(notification.Get());
  if (FAILED(hr)) {
    std::string err = base::StrCat(
        {"WinAPI: Show failed, ERROR ", FailureResultToString(hr)});
    DebugLog(err);
    notif->NotificationFailed(err);
    return;
  }

  DebugLog("Notification created");
  if (notif->delegate())
    notif->delegate()->NotificationDisplayed();
}

// Posts a notification failure event back to the UI thread. If the UI thread's
// task runner is not provided, it fetches it. On the UI thread, calls
// NotificationFailed on the Notification instance (if it is still valid), which
// will invoke the delegate (if set) and clean up.
void WindowsToastNotification::PostNotificationFailedToUIThread(
    base::WeakPtr<Notification> weak_notification,
    const std::string& error,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner) {
  DebugLog(base::StrCat(
      {"PostNotificationFailedToUIThread called with error: ", error}));
  if (!ui_task_runner) {
    ui_task_runner = content::GetUIThreadTaskRunner({});
  }
  ui_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](base::WeakPtr<Notification> weak_notification,
             const std::string& error) {
            DebugLog(
                "PostNotificationFailedToUIThread lambda executing on UI "
                "thread");
            if (!weak_notification) {
              DebugLog(base::StrCat(
                  {"Notification failed but object destroyed: ", error}));
              return;
            }

            // Call NotificationFailed - it will check for delegate internally
            // and emit the event if delegate is set
            DebugLog(base::StrCat(
                {"Calling NotificationFailed with error: ", error}));
            auto* delegate = weak_notification->delegate();
            DebugLog(
                base::StrCat({"Delegate is set: ", delegate ? "yes" : "no"}));

            // Call NotificationFailed which will call delegate if set, then
            // cleanup
            weak_notification->NotificationFailed(error);
            DebugLog("NotificationFailed call completed");
          },
          weak_notification, error));
}

void WindowsToastNotification::Remove() {
  DebugLog("Removing notification from action center");

  ComPtr<winui::Notifications::IToastNotificationManagerStatics2>
      toast_manager2;
  if (FAILED(toast_manager_->As(&toast_manager2)))
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

  (*toast_notifier_)->Hide(toast_notification_.Get());
}

std::u16string WindowsToastNotification::GetToastXml(
    const std::string& notification_id,
    const std::u16string& title,
    const std::u16string& msg,
    const std::wstring& icon_path,
    const std::u16string& timeout_type,
    bool silent,
    const std::vector<NotificationAction>& actions,
    bool has_reply,
    const std::u16string& reply_placeholder) {
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
  xml_writer.AddAttribute(kTemplate, kToastTemplate);

  std::u16string line1;
  std::u16string line2;
  if (title.empty() || msg.empty()) {
    line1 = title.empty() ? msg : title;
    if (line1.empty())
      line1 = u"[no message]";
    // <text>
    xml_writer.StartElement(kText);
    xml_writer.AppendElementContent(base::UTF16ToUTF8(line1));
    xml_writer.EndElement();  // </text>
  } else {
    line1 = title;
    line2 = msg;
    // <text>
    xml_writer.StartElement(kText);
    xml_writer.AppendElementContent(base::UTF16ToUTF8(line1));
    xml_writer.EndElement();  // </text>
    // <text>
    xml_writer.StartElement(kText);
    xml_writer.AppendElementContent(base::UTF16ToUTF8(line2));
    xml_writer.EndElement();  // </text>
  }

  if (!icon_path.empty()) {
    // <image>
    xml_writer.StartElement(kImage);
    xml_writer.AddAttribute(kID, "1");
    xml_writer.AddAttribute(kPlacement, kAppLogoOverride);
    xml_writer.AddAttribute(kHintCrop, kHintCropNone);
    xml_writer.AddAttribute(kSrc, base::WideToUTF8(icon_path));
    xml_writer.EndElement();  // </image>
  }

  xml_writer.EndElement();  // </binding>
  xml_writer.EndElement();  // </visual>

  if (is_reminder || has_reply || !actions.empty()) {
    // <actions>
    xml_writer.StartElement(kActions);
    if (is_reminder) {
      xml_writer.StartElement(kAction);
      xml_writer.AddAttribute(kActivationType, kActivationTypeSystem);
      xml_writer.AddAttribute(kArguments, kDismiss);
      xml_writer.AddAttribute(
          kContent, base::WideToUTF8(l10n_util::GetWideString(IDS_APP_CLOSE)));
      xml_writer.EndElement();  // </action>
    }

    if (has_reply) {
      // <input>
      xml_writer.StartElement(kInput);
      xml_writer.AddAttribute(kID, kReply);
      xml_writer.AddAttribute(kType, kText);
      if (!reply_placeholder.empty()) {
        xml_writer.AddAttribute(kPlaceholderContent,
                                base::UTF16ToUTF8(reply_placeholder));
      }
      xml_writer.EndElement();  // </input>
    }

    for (size_t i = 0; i < actions.size(); ++i) {
      const auto& act = actions[i];
      if (act.type == u"button" || act.type.empty()) {
        // <action>
        xml_writer.StartElement(kAction);
        xml_writer.AddAttribute(kActivationType, kActivationTypeForeground);
        std::string args = base::StrCat(
            {"type=action&action=", base::NumberToString(i),
             "&tag=", base::NumberToString(base::FastHash(notification_id))});
        xml_writer.AddAttribute(kArguments, args.c_str());
        xml_writer.AddAttribute(kContent, base::UTF16ToUTF8(act.text));
        xml_writer.EndElement();  // <action>
      } else if (act.type == u"selection") {
        std::string input_id =
            base::StrCat({kSelection, base::NumberToString(i)});
        xml_writer.StartElement(kInput);  // <input>
        xml_writer.AddAttribute(kID, input_id.c_str());
        xml_writer.AddAttribute(kType, kSelection);
        for (size_t opt_i = 0; opt_i < act.items.size(); ++opt_i) {
          xml_writer.StartElement(kSelection);  // <selection>
          xml_writer.AddAttribute(kID, base::NumberToString(opt_i).c_str());
          xml_writer.AddAttribute(kContent,
                                  base::UTF16ToUTF8(act.items[opt_i]));
          xml_writer.EndElement();  // </selection>
        }
        xml_writer.EndElement();  // </input>

        // The button that submits the selection.
        xml_writer.StartElement(kAction);
        xml_writer.AddAttribute(kActivationType, kActivationTypeForeground);
        std::string args = base::StrCat(
            {"type=action&action=", base::NumberToString(i),
             "&tag=", base::NumberToString(base::FastHash(notification_id))});
        xml_writer.AddAttribute(kArguments, args.c_str());
        xml_writer.AddAttribute(
            kContent,
            base::UTF16ToUTF8(act.text.empty() ? u"Select" : act.text));
        xml_writer.AddAttribute(kHintInputId, input_id.c_str());
        xml_writer.EndElement();  // </action>
      }
    }

    if (has_reply) {
      // <action>
      xml_writer.StartElement(kAction);
      xml_writer.AddAttribute(kActivationType, kActivationTypeForeground);
      std::string args =
          base::StrCat({"type=reply&tag=",
                        base::NumberToString(base::FastHash(notification_id))});
      xml_writer.AddAttribute(kArguments, args.c_str());
      // TODO(codebytere): we should localize this.
      xml_writer.AddAttribute(kContent, "Reply");
      xml_writer.AddAttribute(kHintInputId, kReply);
      xml_writer.EndElement();  // <action>
    }
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
  std::wstring arguments_w;
  std::wstring tag_w;
  std::wstring group_w;

  if (args) {
    Microsoft::WRL::ComPtr<winui::Notifications::IToastActivatedEventArgs>
        activated_args;
    if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&activated_args)))) {
      HSTRING args_hs = nullptr;
      if (SUCCEEDED(activated_args->get_Arguments(&args_hs)) && args_hs) {
        UINT32 len = 0;
        const wchar_t* raw = WindowsGetStringRawBuffer(args_hs, &len);
        if (raw && len)
          arguments_w.assign(raw, len);
      }
    }
  }

  if (sender) {
    Microsoft::WRL::ComPtr<winui::Notifications::IToastNotification2> toast2;
    if (SUCCEEDED(sender->QueryInterface(IID_PPV_ARGS(&toast2)))) {
      HSTRING tag_hs = nullptr;
      if (SUCCEEDED(toast2->get_Tag(&tag_hs)) && tag_hs) {
        UINT32 len = 0;
        const wchar_t* raw = WindowsGetStringRawBuffer(tag_hs, &len);
        if (raw && len)
          tag_w.assign(raw, len);
      }
      HSTRING group_hs = nullptr;
      if (SUCCEEDED(toast2->get_Group(&group_hs)) && group_hs) {
        UINT32 len = 0;
        const wchar_t* raw = WindowsGetStringRawBuffer(group_hs, &len);
        if (raw && len)
          group_w.assign(raw, len);
      }
    }
  }

  std::string notif_id;
  std::string notif_hash;
  if (notification_) {
    notif_id = notification_->notification_id();
    notif_hash = base::NumberToString(base::FastHash(notif_id));
  }

  bool structured = arguments_w.find(L"&tag=") != std::wstring::npos ||
                    arguments_w.find(L"type=action") != std::wstring::npos ||
                    arguments_w.find(L"type=reply") != std::wstring::npos;
  if (structured)
    return S_OK;

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
  std::string errorMessage = FailureResultToString(error);
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&Notification::NotificationFailed,
                                notification_, errorMessage));
  DebugLog(errorMessage);

  return S_OK;
}

}  // namespace electron
