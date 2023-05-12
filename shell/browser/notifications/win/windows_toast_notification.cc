// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon
// <jason.poon@microsoft.com>. All rights reserved.>
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith
// <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of
// this code
// and released it as MIT to the world.

#include "shell/browser/notifications/win/windows_toast_notification.h"
// feat: Upgrade for headers necessary for persistent notifications
// functionality
#include <NotificationActivationCallback.h>
#include <Psapi.h>
#include <SDKDDKVer.h>
#include <ShObjIdl.h>
#include <Shlobj.h>
#include <combaseapi.h>
#include <mmsystem.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shellapi.h>
#include <strsafe.h>
#include <windows.ui.notifications.h>
#include <wrl\module.h>
#include <wrl\wrappers\corewrappers.h>
#include <algorithm>
#include <iterator>
#include <memory>
#include <sstream>

#include "base/environment.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_handle.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/win/notification_presenter_win.h"
#include "shell/browser/win/scoped_hstring.h"
#include "shell/common/application_info.h"
#include "ui/base/l10n/l10n_util_win.h"
#include "ui/strings/grit/ui_strings.h"
// This is win part of Electon and some API requires native StrCat
// so we have problems du to ./src/base/win/windows_defines.inc
// there is some workaround to allow compilation of win part
#undef StrCat
#include "shell/browser/api/electron_api_app.h"

using ABI::Windows::Data::Xml::Dom::IXmlAttribute;
using ABI::Windows::Data::Xml::Dom::IXmlDocument;
using ABI::Windows::Data::Xml::Dom::IXmlDocumentIO;
using ABI::Windows::Data::Xml::Dom::IXmlElement;
using ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap;
using ABI::Windows::Data::Xml::Dom::IXmlNode;
using ABI::Windows::Data::Xml::Dom::IXmlNodeList;
using ABI::Windows::Data::Xml::Dom::IXmlText;
using ABI::Windows::Foundation::IPropertyValue;
using ABI::Windows::Foundation::Collections::IMap;
using ABI::Windows::Foundation::Collections::IMapView;
using ABI::Windows::Foundation::Collections::IPropertySet;
using ABI::Windows::UI::Notifications::IToastActivatedEventArgs;
using ABI::Windows::UI::Notifications::IToastActivatedEventArgs2;
using ABI::Windows::UI::Notifications::ToastDismissalReason;
using base::win::ScopedHandle;
using Microsoft::WRL::Wrappers::HandleT;
using Microsoft::WRL::Wrappers::HStringReference;

//  Used to CoCreate an INotificationActivationCallback interface to notify
//  about toast activations.
EXTERN_C const PROPERTYKEY DECLSPEC_SELECTANY
    PKEY_AppUserModel_ToastActivatorCLSID = {
        {0x9F4C2855,
         0x9F79,
         0x4B39,
         {0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3}},
        26};
using Microsoft::WRL::Wrappers::HStringReference;

#define RETURN_IF_FAILED(hr) \
  do {                       \
    HRESULT _hrTemp = hr;    \
    if (FAILED(_hrTemp)) {   \
      return _hrTemp;        \
    }                        \
  } while (false)
#define REPORT_AND_RETURN_IF_FAILED(hr, msg)                             \
  do {                                                                   \
    HRESULT _hrTemp = hr;                                                \
    std::string _msgTemp = msg;                                          \
    if (FAILED(_hrTemp)) {                                               \
      std::string _err = _msgTemp + ",ERROR " + std::to_string(_hrTemp); \
      if (IsDebuggingNotifications())                                    \
        LOG(INFO) << _err;                                               \
      Notification::NotificationFailed(_err);                            \
      return _hrTemp;                                                    \
    }                                                                    \
  } while (false)

namespace std {
wostream& operator<<(wostream& out, const NOTIFICATION_USER_INPUT_DATA& data) {
  // feat: Refine notification-activation for cold start
  out << L"{";  // open brace for key-value pair
  out << L"\"" << data.Key << L"\"";
  out << L":";  // json delimeter (i.e colon) between key value
  out << L"\"" << data.Value << L"\"";
  out << L"}";  // close brace for key-value pair
  return out;
}
}  // namespace std

namespace electron {

namespace {

bool IsDebuggingNotifications() {
  return base::Environment::Create()->HasVar("ELECTRON_DEBUG_NOTIFICATIONS");
}
// feat: Clear notifications
std::wstring HexStringFromStdHash(std::size_t hash) {
  std::wstringstream ss;
  ss << std::hex << hash;
  return ss.str();
}

}  // namespace

// static
ComPtr<ABI::Windows::UI::Notifications::IToastNotificationManagerStatics>
    WindowsToastNotification::toast_manager_;

// static
ComPtr<ABI::Windows::UI::Notifications::IToastNotificationManagerStatics2>
    ToastEventHandler::toast_manager_;

// static
ComPtr<ABI::Windows::UI::Notifications::IToastNotifier>
    WindowsToastNotification::toast_notifier_;
// feat: Add runtime(dynamic) COM server regestration
class NotificationActivator;
class NotificationActivatorFactory;
class NotificationRegistrator {
 public:
  bool RegisterAppForNotificationSupport(const std::wstring& appId,
                                         const std::wstring& clsid,
                                         const std::wstring& disp);
  bool SetRegistryKeyValue(HKEY hKey,
                           const std::wstring& subKey,
                           const std::wstring& valueName,
                           const std::wstring& value) {
    return SUCCEEDED(HRESULT_FROM_WIN32(::RegSetKeyValue(
        hKey, subKey.c_str(), valueName.empty() ? nullptr : valueName.c_str(),
        REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()),
        static_cast<DWORD>(value.length()) * sizeof(WCHAR))));
  }

  HSTRING AppId() { return app_id_; }
  // feat: Allow App icon into Action center and System Notifications
  // settings
  NotificationRegistrator() {
    wchar_t appPath[MAX_PATH] = {0};
    if (!::GetModuleFileName(nullptr, appPath, ARRAYSIZE(appPath)))
      return;
    app_path_ = std::wstring(appPath);

    wchar_t tmpPath[MAX_PATH] = {0};
    if (!::GetTempPath(ARRAYSIZE(tmpPath), tmpPath))
      return;
    tmp_path_ = std::wstring(tmpPath);
  }

 private:
  ScopedHString app_id_;
  // feat: Allow App icon into Action center and System Notifications
  // settings
  std::wstring app_path_;
  std::wstring tmp_path_;
  std::wstring ico_path_;

  bool ExtractAppIconToTempFile(const std::wstring& file_name);
};

// static
static std::unique_ptr<NotificationRegistrator> registrator_;
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
  if (FAILED(toast_manager_->QueryInterface(
          IID_PPV_ARGS(&(ToastEventHandler::toast_manager_)))))
    return false;

  if (IsRunningInDesktopBridge()) {
    // Ironically, the Desktop Bridge / UWP environment
    // requires us to not give Windows an appUserModelId.
    return SUCCEEDED(toast_manager_->CreateToastNotifier(&toast_notifier_));
  } else {
    ScopedHString app_id;
    if (!GetAppUserModelID(&app_id))
      return false;
    // feat: Support COM activation registration at runtime
    std::wstring notificationsCOMServerCLSID;
    // feat: Application name displays in incorrect format on notification
    std::wstring notificationsCOMDisplayName;
    // feat: Make the com registration system activable
    auto* client = ElectronBrowserClient::Get();
    if (client) {
      const auto& clsid_str = client->GetNotificationsComServerCLSID();
      std::transform(clsid_str.begin(), clsid_str.end(),
                     std::back_inserter(notificationsCOMServerCLSID),
                     [](const char ch) { return ch; });

      const auto& disp_str = client->GetNotificationsComDisplayName();
      std::transform(disp_str.begin(), disp_str.end(),
                     std::back_inserter(notificationsCOMDisplayName),
                     [](const char ch) { return ch; });
    }
    GUID clsid{};
    if (!notificationsCOMServerCLSID.empty() &&
        SUCCEEDED(
            CLSIDFromString(notificationsCOMServerCLSID.c_str(), &clsid))) {
      // feat: Add runtime(dynamic) COM server regestration
      DWORD registration{};
      // Register callback
      if (FAILED(CoRegisterClassObject(
              clsid,
              reinterpret_cast<LPUNKNOWN>(
                  Make<electron::NotificationActivatorFactory>().Get()),
              CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &registration)))
        return false;
      // feat: Support COM server regestration
      registrator_ = std::make_unique<NotificationRegistrator>();
      if (!registrator_->RegisterAppForNotificationSupport(
              WindowsGetStringRawBuffer((HSTRING)app_id, NULL),
              notificationsCOMServerCLSID, notificationsCOMDisplayName))
        return false;
    }
    return SUCCEEDED(
        toast_manager_->CreateToastNotifierWithId(app_id, &toast_notifier_));
  }
}  // namespace electron

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
    if (IsDebuggingNotifications())
      LOG(INFO) << "Notification created";

    if (delegate())
      delegate()->NotificationDisplayed();
  }
}

void WindowsToastNotification::Dismiss() {
  if (IsDebuggingNotifications())
    LOG(INFO) << "Hiding notification";
  toast_notifier_->Hide(toast_notification_.Get());
}

HRESULT WindowsToastNotification::ShowInternal(
    const NotificationOptions& options) {
  if (options.is_persistent && !registrator_) {
    NotificationFailed("COM Activator was not registered");
    return E_FAIL;
  }
  ComPtr<IXmlDocument> toast_xml;
  // The custom xml takes priority over the preset template.
  if (!options.toast_xml.empty()) {
    REPORT_AND_RETURN_IF_FAILED(
        XmlDocumentFromString(base::UTF16ToWide(options.toast_xml).c_str(),
                              &toast_xml),
        "XML: Invalid XML");
  } else {
    REPORT_AND_RETURN_IF_FAILED(
        XmlDocumentFromString(
            L"<toast><visual><binding "
            L"template=\"ToastGeneric\"></binding></visual></toast>",
            &toast_xml),
        "XML: FAILED during setup ToastGeneric scheme");

    auto* presenter_win = static_cast<NotificationPresenterWin*>(presenter());
    std::wstring icon_path =
        presenter_win->SaveIconToFilesystem(options.icon, options.icon_url);

    std::wstring image_path =
        presenter_win->SaveIconToFilesystem(options.image, options.image_url);

    // feat: Add the reply field on a notification
    // Win XML toast scheme requires that inputs will follow befor buttons
    // code bellow provide smart sorting without lost of initial inputs/buttons
    // relative order
    std::vector<electron::NotificationAction> actions;
    auto add_inputs =
        [&actions](const electron::NotificationAction& notification) {
          if (notification.type == NotificationAction::sTYPE_TEXT)
            actions.push_back(notification);
        };
    auto add_buttons =
        [&actions](const electron::NotificationAction& notification) {
          if (notification.type == NotificationAction::sTYPE_BUTTON)
            actions.push_back(notification);
        };
    std::for_each(options.actions.begin(), options.actions.end(), add_inputs);
    std::for_each(options.actions.begin(), options.actions.end(), add_buttons);

    REPORT_AND_RETURN_IF_FAILED(
        SetToastXml(toast_xml.Get(), base::UTF16ToWide(options.title),
                    base::UTF16ToWide(options.msg), icon_path, image_path,
                    base::UTF16ToWide(options.timeout_type),
                    base::UTF16ToWide(options.data), actions, options.silent,
                    options.require_interaction),
        "XML: Failed to create XML document");
  }

  ScopedHString toast_str(
      RuntimeClass_Windows_UI_Notifications_ToastNotification);
  if (!toast_str.success()) {
    NotificationFailed("Creating ScopedHString failed");
    return E_FAIL;
  }

  ComPtr<ABI::Windows::UI::Notifications::IToastNotificationFactory>
      toast_factory;
  REPORT_AND_RETURN_IF_FAILED(
      Windows::Foundation::GetActivationFactory(toast_str, &toast_factory),
      "WinAPI: GetActivationFactory failed");

  REPORT_AND_RETURN_IF_FAILED(toast_factory->CreateToastNotification(
                                  toast_xml.Get(), &toast_notification_),
                              "WinAPI: CreateToastNotification failed");

  ComPtr<ABI::Windows::UI::Notifications::IToastNotification2>
      toast_notification2_;

  REPORT_AND_RETURN_IF_FAILED(
      toast_notification_->QueryInterface(IID_PPV_ARGS(&toast_notification2_)),
      "WinAPI: CreateToastNotification2 failed");

  // https://docs.microsoft.com/en-us/uwp/api/windows.ui.notifications.toastnotification.tag?view=winrt-20348
  // The tag can be maximum 16 characters long. However,
  // the Creators Update (15063) extends this limit to 64 characters.
  const std::wstring& hex_str{
      HexStringFromStdHash(std::hash<std::string>{}(notification_id()))};
  // Beware: do not put any methods with temporary wstring as return value
  // directly to HStringReference c-tor HStringReference uses bare pointer to
  // raw std::wstring data which should be available during all scope
  // life-circle
  REPORT_AND_RETURN_IF_FAILED(
      toast_notification2_->put_Tag(HStringReference(hex_str.c_str()).Get()),
      "WinAPI: put_Tag failed");

  // feat: Display toast according to Notification.renotify
  REPORT_AND_RETURN_IF_FAILED(
      toast_notification2_->put_SuppressPopup(!options.should_be_presented),
      "WinAPI: setup put_Suppress failed");

  REPORT_AND_RETURN_IF_FAILED(SetupCallbacks(toast_notification_.Get()),
                              "WinAPI: SetupCallbacks failed");

  if (options.require_interaction && !options.silent) {
    // feat: Can not require interaction on Windows
    // Workaround to silent system IncommingCall sound and play Default sound by
    // custom PlaySound call
    // https://docs.microsoft.com/en-us/previous-versions/dd743680(v=vs.85)
    PlaySound((LPCTSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ASYNC | SND_ALIAS_ID);
  }

  REPORT_AND_RETURN_IF_FAILED(
      (event_handler_->SetNotificationOptions(options),
       toast_notifier_->Show(toast_notification_.Get())),
      "WinAPI: Show failed");
  return S_OK;
}

PCWSTR WindowsToastNotification::AsString(
    const ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument>& xmlDocument) {
  HSTRING xml;
  ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeSerializer> ser;
  HRESULT hr =
      xmlDocument.As<ABI::Windows::Data::Xml::Dom::IXmlNodeSerializer>(&ser);
  hr = ser->GetXml(&xml);
  if (SUCCEEDED(hr))
    return WindowsGetStringRawBuffer(xml, nullptr);
  return nullptr;
}

static const std::wstring kFileUriPrefix{L"file:///"};
HRESULT WindowsToastNotification::SetToastXml(
    IXmlDocument* toast_xml,
    const std::wstring& title,
    const std::wstring& msg,
    const std::wstring& icon_path,
    const std::wstring& image_path,
    const std::wstring& timeout_type,
    const std::wstring& data,
    const std::vector<electron::NotificationAction>& actions_list,
    const bool silent,
    const bool require_interaction) {
  // Fill the toast's title
  if (!title.empty()) {
    REPORT_AND_RETURN_IF_FAILED(SetXmlText(toast_xml, title),
                                "XML: Set toast title failed");
  }

  // Fill the toast's message
  if (!msg.empty()) {
    REPORT_AND_RETURN_IF_FAILED(SetXmlText(toast_xml, msg),
                                "XML: Set toast message failed");
  }

  // Configure the toast's timeout settings
  if (timeout_type == base::ASCIIToWide("never")) {
    REPORT_AND_RETURN_IF_FAILED(
        (SetXmlScenarioReminder(toast_xml)),
        "XML: Setting \"scenario\" option on notification failed");
  }

  if (require_interaction) {
    const bool use_reminder =
        !actions_list.empty() &&
        actions_list[0].type != NotificationAction::sTYPE_TEXT;
    REPORT_AND_RETURN_IF_FAILED(
        SetXmlScenarioType(
            toast_xml,
            std::wstring(use_reminder ? L"reminder" : L"incomingcall")),
        "XML: Setting \"reminder\" scenario type failed");
  }

  // Configure the toast's notification sound
  if (silent || require_interaction) {
    // feat: Can not require interaction on Windows
    // Workaround to silent system IncommingCall sound and play Default sound by
    // custom PlaySound call
    // feat: Notification showing up with strange sounds when
    // requireInteraction is true require_interaction is necessary to fix side
    // effect from applying incomingcall
    REPORT_AND_RETURN_IF_FAILED(
        SetXmlAudioSilent(toast_xml),
        "XML: Setting \"silent\" option on notification failed");
  }

  // Configure the toast's image
  if (!image_path.empty()) {
    REPORT_AND_RETURN_IF_FAILED(
        SetXmlImage(toast_xml, image_path, L"hero"),
        "XML: Setting \"icon\" option on notification failed");
  }

  // Configure the toast's icon
  if (!icon_path.empty()) {
    REPORT_AND_RETURN_IF_FAILED(
        SetXmlImage(toast_xml, icon_path, L"appLogoOverride"),
        "XML: Setting \"image\" option on notification failed");
  }

  if (!data.empty()) {
    REPORT_AND_RETURN_IF_FAILED(
        SetLaunchParams(toast_xml, data),
        "XML: Setting \"data\" option on notification failed");
  }

  for (const auto& action : actions_list) {
    std::wstring icon_path_ = base::UTF8ToWide(action.icon.spec());
    if (icon_path_.find(kFileUriPrefix) == 0) {
      icon_path_.assign(icon_path_, kFileUriPrefix.length(),
                        std::wstring::npos);
    }
    std::wstring hint_input_id_;
    REPORT_AND_RETURN_IF_FAILED(
        AddAction(toast_xml, base::UTF16ToWide(action.type),
                  base::UTF16ToWide(action.text), base::UTF16ToWide(action.arg),
                  icon_path_, base::UTF16ToWide(action.placeholder),
                  hint_input_id_),
        "XML: Setting \"action\" option on notification failed");
    if (action.type != NotificationAction::sTYPE_TEXT)
      continue;
    hint_input_id_ = base::UTF16ToWide(action.arg);
    REPORT_AND_RETURN_IF_FAILED(
        AddAction(
            toast_xml, base::UTF16ToWide(NotificationAction::sTYPE_BUTTON),
            base::UTF16ToWide(action.text),
            base::UTF16ToWide(action.arg + NotificationAction::sTYPE_BUTTON),
            icon_path_, base::UTF16ToWide(action.placeholder), hint_input_id_),
        "XML: Setting \"action\" option on notification failed");
  }
  // Next approach can be used for accessing to XML toast scheme during debug
  // {
  // ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument> doc;
  // doc.Attach(toast_xml);
  // PCWSTR str = AsString(doc);
  // OutputDebugString(str);
  // OutputDebugString(L"\n");
  // doc.Detach();
  // }

  return S_OK;
}

HRESULT WindowsToastNotification::SetXmlScenarioReminder(IXmlDocument* doc) {
  ScopedHString tag(L"toast");
  if (!tag.success())
    return false;

  ComPtr<IXmlNodeList> node_list;
  RETURN_IF_FAILED(doc->GetElementsByTagName(tag, &node_list));

  // Check that root "toast" node exists
  ComPtr<IXmlNode> root;
  RETURN_IF_FAILED(node_list->Item(0, &root));

  // get attributes of root "toast" node
  ComPtr<IXmlNamedNodeMap> toast_attributes;
  RETURN_IF_FAILED(root->get_Attributes(&toast_attributes));

  ComPtr<IXmlAttribute> scenario_attribute;
  ScopedHString scenario_str(L"scenario");
  RETURN_IF_FAILED(doc->CreateAttribute(scenario_str, &scenario_attribute));

  ComPtr<IXmlNode> scenario_attribute_node;
  RETURN_IF_FAILED(scenario_attribute.As(&scenario_attribute_node));

  ScopedHString scenario_value(L"reminder");
  if (!scenario_value.success())
    return E_FAIL;

  ComPtr<IXmlText> scenario_text;
  RETURN_IF_FAILED(doc->CreateTextNode(scenario_value, &scenario_text));

  ComPtr<IXmlNode> scenario_node;
  RETURN_IF_FAILED(scenario_text.As(&scenario_node));

  ComPtr<IXmlNode> scenario_backup_node;
  RETURN_IF_FAILED(scenario_attribute_node->AppendChild(scenario_node.Get(),
                                                        &scenario_backup_node));

  ComPtr<IXmlNode> scenario_attribute_pnode;
  RETURN_IF_FAILED(toast_attributes.Get()->SetNamedItem(
      scenario_attribute_node.Get(), &scenario_attribute_pnode));

  // Create "actions" wrapper
  ComPtr<IXmlElement> actions_wrapper_element;
  ScopedHString actions_wrapper_str(L"actions");
  RETURN_IF_FAILED(
      doc->CreateElement(actions_wrapper_str, &actions_wrapper_element));

  ComPtr<IXmlNode> actions_wrapper_node_tmp;
  RETURN_IF_FAILED(actions_wrapper_element.As(&actions_wrapper_node_tmp));

  // Append actions wrapper node to toast xml
  ComPtr<IXmlNode> actions_wrapper_node;
  RETURN_IF_FAILED(
      root->AppendChild(actions_wrapper_node_tmp.Get(), &actions_wrapper_node));

  ComPtr<IXmlNamedNodeMap> attributes_actions_wrapper;
  RETURN_IF_FAILED(
      actions_wrapper_node->get_Attributes(&attributes_actions_wrapper));

  // Add a "Dismiss" button
  // Create "action" tag
  ComPtr<IXmlElement> action_element;
  ScopedHString action_str(L"action");
  RETURN_IF_FAILED(doc->CreateElement(action_str, &action_element));

  ComPtr<IXmlNode> action_node_tmp;
  RETURN_IF_FAILED(action_element.As(&action_node_tmp));

  // Append action node to actions wrapper in toast xml
  ComPtr<IXmlNode> action_node;
  RETURN_IF_FAILED(
      actions_wrapper_node->AppendChild(action_node_tmp.Get(), &action_node));

  // Setup attributes for action
  ComPtr<IXmlNamedNodeMap> action_attributes;
  RETURN_IF_FAILED(action_node->get_Attributes(&action_attributes));

  // Create activationType attribute
  ComPtr<IXmlAttribute> activation_type_attribute;
  ScopedHString activation_type_str(L"activationType");
  RETURN_IF_FAILED(
      doc->CreateAttribute(activation_type_str, &activation_type_attribute));

  ComPtr<IXmlNode> activation_type_attribute_node;
  RETURN_IF_FAILED(
      activation_type_attribute.As(&activation_type_attribute_node));

  // Set activationType attribute to system
  ScopedHString activation_type_value(L"system");
  if (!activation_type_value.success())
    return E_FAIL;

  ComPtr<IXmlText> activation_type_text;
  RETURN_IF_FAILED(
      doc->CreateTextNode(activation_type_value, &activation_type_text));

  ComPtr<IXmlNode> activation_type_node;
  RETURN_IF_FAILED(activation_type_text.As(&activation_type_node));

  ComPtr<IXmlNode> activation_type_backup_node;
  RETURN_IF_FAILED(activation_type_attribute_node->AppendChild(
      activation_type_node.Get(), &activation_type_backup_node));

  // Add activation type to the action attributes
  ComPtr<IXmlNode> activation_type_attribute_pnode;
  RETURN_IF_FAILED(action_attributes.Get()->SetNamedItem(
      activation_type_attribute_node.Get(), &activation_type_attribute_pnode));

  // Create arguments attribute
  ComPtr<IXmlAttribute> arguments_attribute;
  ScopedHString arguments_str(L"arguments");
  RETURN_IF_FAILED(doc->CreateAttribute(arguments_str, &arguments_attribute));

  ComPtr<IXmlNode> arguments_attribute_node;
  RETURN_IF_FAILED(arguments_attribute.As(&arguments_attribute_node));

  // Set arguments attribute to dismiss
  ScopedHString arguments_value(L"dismiss");
  if (!arguments_value.success())
    return E_FAIL;

  ComPtr<IXmlText> arguments_text;
  RETURN_IF_FAILED(doc->CreateTextNode(arguments_value, &arguments_text));

  ComPtr<IXmlNode> arguments_node;
  RETURN_IF_FAILED(arguments_text.As(&arguments_node));

  ComPtr<IXmlNode> arguments_backup_node;
  RETURN_IF_FAILED(arguments_attribute_node->AppendChild(
      arguments_node.Get(), &arguments_backup_node));

  // Add arguments to the action attributes
  ComPtr<IXmlNode> arguments_attribute_pnode;
  RETURN_IF_FAILED(action_attributes.Get()->SetNamedItem(
      arguments_attribute_node.Get(), &arguments_attribute_pnode));

  // Create content attribute
  ComPtr<IXmlAttribute> content_attribute;
  ScopedHString content_str(L"content");
  RETURN_IF_FAILED(doc->CreateAttribute(content_str, &content_attribute));

  ComPtr<IXmlNode> content_attribute_node;
  RETURN_IF_FAILED(content_attribute.As(&content_attribute_node));

  // Set content attribute to Dismiss
  ScopedHString content_value(l10n_util::GetWideString(IDS_APP_CLOSE));
  if (!content_value.success())
    return E_FAIL;

  ComPtr<IXmlText> content_text;
  RETURN_IF_FAILED(doc->CreateTextNode(content_value, &content_text));

  ComPtr<IXmlNode> content_node;
  RETURN_IF_FAILED(content_text.As(&content_node));

  ComPtr<IXmlNode> content_backup_node;
  RETURN_IF_FAILED(content_attribute_node->AppendChild(content_node.Get(),
                                                       &content_backup_node));

  // Add content to the action attributes
  ComPtr<IXmlNode> content_attribute_pnode;
  return action_attributes.Get()->SetNamedItem(content_attribute_node.Get(),
                                               &content_attribute_pnode);
}

HRESULT WindowsToastNotification::SetXmlScenarioType(
    ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
    const std::wstring& scenario_type) {
  ScopedHString tag(L"toast");
  if (!tag.success())
    return false;

  ComPtr<IXmlNodeList> node_list;
  RETURN_IF_FAILED(doc->GetElementsByTagName(tag, &node_list));

  // Check that root "toast" node exists
  ComPtr<IXmlNode> root;
  RETURN_IF_FAILED(node_list->Item(0, &root));

  // get attributes of root "toast" node
  ComPtr<IXmlNamedNodeMap> toast_attributes;
  RETURN_IF_FAILED(root->get_Attributes(&toast_attributes));

  ComPtr<IXmlAttribute> scenario_attribute;
  ScopedHString scenario_str(L"scenario");
  RETURN_IF_FAILED(doc->CreateAttribute(scenario_str, &scenario_attribute));

  ComPtr<IXmlNode> scenario_attribute_node;
  RETURN_IF_FAILED(scenario_attribute.As(&scenario_attribute_node));

  ScopedHString scenario_value(scenario_type);
  if (!scenario_value.success())
    return E_FAIL;

  ComPtr<IXmlText> scenario_text;
  RETURN_IF_FAILED(doc->CreateTextNode(scenario_value, &scenario_text));

  ComPtr<IXmlNode> scenario_node;
  RETURN_IF_FAILED(scenario_text.As(&scenario_node));

  ComPtr<IXmlNode> scenario_backup_node;
  RETURN_IF_FAILED(scenario_attribute_node->AppendChild(scenario_node.Get(),
                                                        &scenario_backup_node));

  ComPtr<IXmlNode> scenario_attribute_pnode;
  return toast_attributes.Get()->SetNamedItem(scenario_attribute_node.Get(),
                                              &scenario_attribute_pnode);
}

HRESULT WindowsToastNotification::SetXmlAudioSilent(IXmlDocument* doc) {
  ScopedHString tag(L"toast");
  if (!tag.success())
    return E_FAIL;

  ComPtr<IXmlNodeList> node_list;
  RETURN_IF_FAILED(doc->GetElementsByTagName(tag, &node_list));

  ComPtr<IXmlNode> root;
  RETURN_IF_FAILED(node_list->Item(0, &root));

  ComPtr<IXmlElement> audio_element;
  ScopedHString audio_str(L"audio");
  RETURN_IF_FAILED(doc->CreateElement(audio_str, &audio_element));

  ComPtr<IXmlNode> audio_node_tmp;
  RETURN_IF_FAILED(audio_element.As(&audio_node_tmp));

  // Append audio node to toast xml
  ComPtr<IXmlNode> audio_node;
  RETURN_IF_FAILED(root->AppendChild(audio_node_tmp.Get(), &audio_node));

  // Create silent attribute
  ComPtr<IXmlNamedNodeMap> attributes;
  RETURN_IF_FAILED(audio_node->get_Attributes(&attributes));

  ComPtr<IXmlAttribute> silent_attribute;
  ScopedHString silent_str(L"silent");
  RETURN_IF_FAILED(doc->CreateAttribute(silent_str, &silent_attribute));

  ComPtr<IXmlNode> silent_attribute_node;
  RETURN_IF_FAILED(silent_attribute.As(&silent_attribute_node));

  // Set silent attribute to true
  ScopedHString silent_value(L"true");
  if (!silent_value.success())
    return E_FAIL;

  ComPtr<IXmlText> silent_text;
  RETURN_IF_FAILED(doc->CreateTextNode(silent_value, &silent_text));

  ComPtr<IXmlNode> silent_node;
  RETURN_IF_FAILED(silent_text.As(&silent_node));

  ComPtr<IXmlNode> child_node;
  RETURN_IF_FAILED(
      silent_attribute_node->AppendChild(silent_node.Get(), &child_node));

  ComPtr<IXmlNode> silent_attribute_pnode;
  return attributes.Get()->SetNamedItem(silent_attribute_node.Get(),
                                        &silent_attribute_pnode);
}

HRESULT WindowsToastNotification::SetXmlText(IXmlDocument* doc,
                                             const std::wstring& text) {
  ScopedHString tag(L"binding");
  if (!tag.success())
    return E_FAIL;

  ComPtr<IXmlNodeList> node_list;
  RETURN_IF_FAILED(doc->GetElementsByTagName(tag, &node_list));

  ComPtr<IXmlNode> node;
  RETURN_IF_FAILED(node_list->Item(0, &node));

  ComPtr<IXmlElement> text_element;
  ScopedHString text_str(L"text");
  RETURN_IF_FAILED(doc->CreateElement(text_str, &text_element));

  ComPtr<IXmlNode> text_node_backup;
  RETURN_IF_FAILED(text_element.As(&text_node_backup));

  // Append text node to toast xml
  ComPtr<IXmlNode> text_node;
  RETURN_IF_FAILED(node->AppendChild(text_node_backup.Get(), &text_node));

  return AppendTextToXml(doc, text_node.Get(), text);
}

HRESULT WindowsToastNotification::SetXmlImage(IXmlDocument* doc,
                                              const std::wstring& icon_path,
                                              const std::wstring& placement) {
  ScopedHString tag(L"binding");
  if (!tag.success())
    return E_FAIL;

  ComPtr<IXmlNodeList> node_list;
  RETURN_IF_FAILED(doc->GetElementsByTagName(tag, &node_list));

  ComPtr<IXmlNode> root;
  RETURN_IF_FAILED(node_list->Item(0, &root));

  ComPtr<IXmlElement> image_element;
  ScopedHString image_str(L"image");
  RETURN_IF_FAILED(doc->CreateElement(image_str, &image_element));

  ComPtr<IXmlNode> image_node_backup;
  RETURN_IF_FAILED(image_element.As(&image_node_backup));

  // Append image node to toast xml
  ComPtr<IXmlNode> image_node;
  RETURN_IF_FAILED(root->AppendChild(image_node_backup.Get(), &image_node));

  // Create image attributes
  ComPtr<IXmlNamedNodeMap> attributes;
  RETURN_IF_FAILED(image_node->get_Attributes(&attributes));

  ComPtr<IXmlAttribute> placement_attribute;
  ScopedHString placement_str(L"placement");
  RETURN_IF_FAILED(doc->CreateAttribute(placement_str, &placement_attribute));

  ComPtr<IXmlNode> placement_attribute_node;
  RETURN_IF_FAILED(placement_attribute.As(&placement_attribute_node));

  ScopedHString placement_value(placement);
  if (!placement_value.success())
    return E_FAIL;

  ComPtr<IXmlText> placement_text;
  RETURN_IF_FAILED(doc->CreateTextNode(placement_value, &placement_text));

  ComPtr<IXmlNode> placement_node;
  RETURN_IF_FAILED(placement_text.As(&placement_node));

  ComPtr<IXmlNode> placement_backup_node;
  RETURN_IF_FAILED(placement_attribute_node->AppendChild(
      placement_node.Get(), &placement_backup_node));

  {
    ComPtr<IXmlNode> scenario_attribute_pnode;
    RETURN_IF_FAILED(attributes.Get()->SetNamedItem(
        placement_attribute_node.Get(), &scenario_attribute_pnode));
  }

  ComPtr<IXmlAttribute> src_attribute;
  ScopedHString src_str(L"src");
  RETURN_IF_FAILED(doc->CreateAttribute(src_str, &src_attribute));

  ComPtr<IXmlNode> src_attribute_node;
  RETURN_IF_FAILED(src_attribute.As(&src_attribute_node));

  ScopedHString img_path(icon_path.c_str());
  if (!img_path.success())
    return E_FAIL;

  ComPtr<IXmlText> src_text;
  RETURN_IF_FAILED(doc->CreateTextNode(img_path, &src_text));

  ComPtr<IXmlNode> src_node;
  RETURN_IF_FAILED(src_text.As(&src_node));

  ComPtr<IXmlNode> src_backup_node;
  RETURN_IF_FAILED(
      src_attribute_node->AppendChild(src_node.Get(), &src_backup_node));

  {
    ComPtr<IXmlNode> scenario_attribute_pnode;
    RETURN_IF_FAILED(attributes.Get()->SetNamedItem(src_attribute_node.Get(),
                                                    &scenario_attribute_pnode));
  }

  return S_OK;
}

HRESULT WindowsToastNotification::SetLaunchParams(
    ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
    const std::wstring& params) {
  UINT32 length;
  ComPtr<IXmlNodeList> nodeList;
  RETURN_IF_FAILED(
      doc->GetElementsByTagName(HStringReference(L"toast").Get(), &nodeList));

  RETURN_IF_FAILED(nodeList->get_Length(&length));

  ComPtr<IXmlNode> toastNode;
  RETURN_IF_FAILED(nodeList->Item(0, &toastNode));

  ComPtr<IXmlElement> toastElement;
  RETURN_IF_FAILED(toastNode.As(&toastElement));

  return toastElement->SetAttribute(HStringReference(L"launch").Get(),
                                    HStringReference(params.c_str()).Get());
}

HRESULT WindowsToastNotification::AppendTextToXml(IXmlDocument* doc,
                                                  IXmlNode* node,
                                                  const std::wstring& text) {
  ScopedHString str(text);
  if (!str.success())
    return E_FAIL;

  ComPtr<IXmlText> xml_text;
  RETURN_IF_FAILED(doc->CreateTextNode(str, &xml_text));

  ComPtr<IXmlNode> text_node;
  RETURN_IF_FAILED(xml_text.As(&text_node));

  ComPtr<IXmlNode> append_node;
  RETURN_IF_FAILED(node->AppendChild(text_node.Get(), &append_node));

  return S_OK;
}

HRESULT WindowsToastNotification::AddAction(
    ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
    const std::wstring& type,
    const std::wstring& content,
    const std::wstring& arguments,  // equal to id for input
    const std::wstring& icon_path,
    const std::wstring& placeholder,
    const std::wstring& hint_inputId) {
  ComPtr<IXmlNodeList> nodeList;
  RETURN_IF_FAILED(
      doc->GetElementsByTagName(HStringReference(L"actions").Get(), &nodeList));
  UINT32 length;
  RETURN_IF_FAILED(nodeList->get_Length(&length));
  ComPtr<IXmlNode> actionsNode;
  if (length > 0) {
    RETURN_IF_FAILED(nodeList->Item(0, &actionsNode));
  } else {
    RETURN_IF_FAILED(
        doc->GetElementsByTagName(HStringReference(L"toast").Get(), &nodeList));

    RETURN_IF_FAILED(nodeList->get_Length(&length));

    ComPtr<IXmlNode> toastNode;
    RETURN_IF_FAILED(nodeList->Item(0, &toastNode));

    ComPtr<IXmlElement> toastElement;
    RETURN_IF_FAILED(toastNode.As(&toastElement));

    ComPtr<IXmlElement> actionsElement;
    RETURN_IF_FAILED(doc->CreateElement(HStringReference(L"actions").Get(),
                                        &actionsElement));

    RETURN_IF_FAILED(actionsElement.As(&actionsNode));

    ComPtr<IXmlNode> appendedChild;
    RETURN_IF_FAILED(toastNode->AppendChild(actionsNode.Get(), &appendedChild));
  }

  ComPtr<IXmlElement> actionElement;
  if (type == L"button") {
    // BUTTON
    RETURN_IF_FAILED(
        doc->CreateElement(HStringReference(L"action").Get(), &actionElement));

    RETURN_IF_FAILED(
        actionElement->SetAttribute(HStringReference(L"content").Get(),
                                    HStringReference(content.c_str()).Get()));

    RETURN_IF_FAILED(
        actionElement->SetAttribute(HStringReference(L"arguments").Get(),
                                    HStringReference(arguments.c_str()).Get()));

    RETURN_IF_FAILED(
        actionElement->SetAttribute(HStringReference(L"imageUri").Get(),
                                    HStringReference(icon_path.c_str()).Get()));
    RETURN_IF_FAILED(actionElement->SetAttribute(
        HStringReference(L"hint-inputId").Get(),
        HStringReference(hint_inputId.c_str()).Get()));
  } else {
    // INPUT
    RETURN_IF_FAILED(
        doc->CreateElement(HStringReference(L"input").Get(), &actionElement));

    RETURN_IF_FAILED(
        actionElement->SetAttribute(HStringReference(L"id").Get(),
                                    HStringReference(arguments.c_str()).Get()));
    RETURN_IF_FAILED(actionElement->SetAttribute(
        HStringReference(L"type").Get(), HStringReference(type.c_str()).Get()));
    RETURN_IF_FAILED(actionElement->SetAttribute(
        HStringReference(L"placeHolderContent").Get(),
        HStringReference(placeholder.c_str()).Get()));
  }

  ComPtr<IXmlNode> actionNode;
  RETURN_IF_FAILED(actionElement.As(&actionNode));

  ComPtr<IXmlNode> appendedChild;
  RETURN_IF_FAILED(actionsNode->AppendChild(actionNode.Get(), &appendedChild));

  return S_OK;
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
    ABI::Windows::UI::Notifications::IToastNotification* toast) {
  event_handler_ = Make<ToastEventHandler>(this);
  RETURN_IF_FAILED(
      toast->add_Activated(event_handler_.Get(), &activated_token_));
  RETURN_IF_FAILED(
      toast->add_Dismissed(event_handler_.Get(), &dismissed_token_));
  RETURN_IF_FAILED(toast->add_Failed(event_handler_.Get(), &failed_token_));
  return S_OK;
}

HRESULT WindowsToastNotification::RemoveCallbacks(
    ABI::Windows::UI::Notifications::IToastNotification* toast) {
  RETURN_IF_FAILED(toast->remove_Activated(activated_token_));

  RETURN_IF_FAILED(toast->remove_Dismissed(dismissed_token_));

  RETURN_IF_FAILED(toast->remove_Failed(failed_token_));
  return S_OK;
}

/*
/ Toast Event Handler
*/
ToastEventHandler::ToastEventHandler(Notification* notification)
    : notification_(notification->GetWeakPtr()) {}

ToastEventHandler::~ToastEventHandler() {}

HRESULT GetReplyFromNotificationAction(
    const ComPtr<IMap<HSTRING, IInspectable*>>& replyMap,
    const electron::NotificationAction& action,
    std::string* reply) {
  // Beware: do not put any methods with temporary wstring as return value
  // directly to HStringReference c-tor
  // HStringReference uses bare pointer to raw std::wstring data which should be
  // available during all scope life-circle
  const std::wstring& action_arg{base::UTF16ToWide(action.arg)};
  HSTRING hKey = HStringReference(action_arg.c_str()).Get();

  boolean bHasKey;
  RETURN_IF_FAILED(replyMap->HasKey(hKey, &bHasKey));

  if (bHasKey) {
    ComPtr<IInspectable> pValue;
    RETURN_IF_FAILED(replyMap->Lookup(hKey, &pValue));

    ComPtr<IPropertyValue> str;
    RETURN_IF_FAILED(pValue.As(&str));

    HSTRING argumentsHandle;
    RETURN_IF_FAILED(str->GetString(&argumentsHandle));
    PCWSTR arguments = WindowsGetStringRawBuffer(argumentsHandle, NULL);

    if (arguments && *arguments)
      *reply = base::WideToUTF8(arguments);
  }
  return S_OK;
}

static const std::wstring kReplyTextType{L"text"};
IFACEMETHODIMP ToastEventHandler::Invoke(
    ABI::Windows::UI::Notifications::IToastNotification* sender,
    IInspectable* args) {
  // feat: Upgrade for get_Activated callback
  if (options_.is_persistent) {
    // click on persistent notification toast
    if (options_.has_reply) {
      // Notification::NotificationReplied scope
      // feat: Add the reply field on a notification
      ComPtr<IToastActivatedEventArgs2> activatedEventArgs2;
      RETURN_IF_FAILED(
          args->QueryInterface(activatedEventArgs2.GetAddressOf()));

      ComPtr<IPropertySet> propSet;
      RETURN_IF_FAILED(activatedEventArgs2->get_UserInput(&propSet));

      ComPtr<IMap<HSTRING, IInspectable*>> replyMap;
      RETURN_IF_FAILED(propSet->QueryInterface(IID_PPV_ARGS(&replyMap)));

      PCWSTR arguments = kReplyTextType.c_str();
      if (arguments && *arguments) {
        auto first = options_.actions.begin();
        auto last = options_.actions.end();
        auto find = std::find_if(
            first, last,
            [arguments](const electron::NotificationAction& action) {
              return base::UTF16ToWide(action.type) == arguments;
            });
        int index = -1;
        if (find != last)  // index of first field with reply
          index = std::distance(first, find);
        // Processing for index == -1 will allow to distinguish click on toast
        // for this case event.reply field will be empty
        std::string reply;
        if (index != -1) {
          RETURN_IF_FAILED(GetReplyFromNotificationAction(
              replyMap, options_.actions[index], &reply));
        }
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE, base::BindOnce(&Notification::NotificationReplied,
                                      notification_, reply));
        if (IsDebuggingNotifications())
          LOG(INFO) << "NotificationReplied";
      }
      // if options_.has_reply
    } else {
      // Notification::NotificationAction scope
      ComPtr<IToastActivatedEventArgs> activatedEventArgs;
      if (FAILED(args->QueryInterface(activatedEventArgs.GetAddressOf())))
        return HRESULT_FROM_WIN32(::GetLastError());
      HSTRING argumentsHandle;
      if (FAILED(activatedEventArgs->get_Arguments(&argumentsHandle)))
        return HRESULT_FROM_WIN32(::GetLastError());
      PCWSTR arguments = WindowsGetStringRawBuffer(argumentsHandle, NULL);
      if (arguments && *arguments) {
        auto first = options_.actions.begin();
        auto last = options_.actions.end();
        auto find = std::find_if(
            first, last,
            [arguments](const electron::NotificationAction& action) {
              return base::UTF16ToWide(action.arg) == arguments;
            });
        int index = -1;
        if (find != last)  // index for toast button clicked
          index = std::distance(first, find);
        // Forwarding index == -1 will allow to distinguish click on toast
        // for this case event.action field will be empty
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE, base::BindOnce(&Notification::NotificationAction,
                                      notification_, index));
        if (IsDebuggingNotifications())
          LOG(INFO) << "NotificationAction";
      }
    }
    // else options_.has_reply
  } else {
    // if options_.is_persistent
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(&Notification::NotificationClicked, notification_));
    if (IsDebuggingNotifications())
      LOG(INFO) << "Notification clicked";
  }

  return S_OK;
}

IFACEMETHODIMP ToastEventHandler::Invoke(
    ABI::Windows::UI::Notifications::IToastNotification* sender,
    ABI::Windows::UI::Notifications::IToastDismissedEventArgs* e) {
  ToastDismissalReason reason;
  // feat: Upgrade for get_Dismissed callback
  if (SUCCEEDED(e->get_Reason(&reason)) &&
      // we need to post dismissed only for that case
      reason == ToastDismissalReason::ToastDismissalReason_UserCanceled) {
    // feat: Clear notifications
    ComPtr<ABI::Windows::UI::Notifications::IToastNotificationHistory> hist;
    RETURN_IF_FAILED(toast_manager_->get_History(&hist));

    ComPtr<ABI::Windows::UI::Notifications::IToastNotificationHistory2> hist2;
    RETURN_IF_FAILED(hist->QueryInterface(IID_PPV_ARGS(&hist2)));

    ComPtr<ABI::Windows::Foundation::Collections::IVectorView<
        ABI::Windows::UI::Notifications::ToastNotification*>>
        hist_view;
    RETURN_IF_FAILED(
        hist2->GetHistoryWithId(registrator_->AppId(), &hist_view));

    unsigned size = 0;
    RETURN_IF_FAILED(hist_view->get_Size(&size));

    ComPtr<ABI::Windows::UI::Notifications::IToastNotification2> notif2;
    RETURN_IF_FAILED(sender->QueryInterface(IID_PPV_ARGS(&notif2)));

    HSTRING ref;
    RETURN_IF_FAILED(notif2->get_Tag(&ref));

    bool notification_is_exisit(false);
    for (unsigned i = 0; !notification_is_exisit && i < size; i++) {
      ComPtr<ABI::Windows::UI::Notifications::IToastNotification> n;
      RETURN_IF_FAILED(hist_view->GetAt(i, &n));

      ComPtr<ABI::Windows::UI::Notifications::IToastNotification2> n2;
      RETURN_IF_FAILED(n->QueryInterface(IID_PPV_ARGS(&n2)));

      HSTRING tag;
      RETURN_IF_FAILED(n2->get_Tag(&tag));

      INT32 result;
      RETURN_IF_FAILED(WindowsCompareStringOrdinal(ref, tag, &result));

      notification_is_exisit = result == 0;
    }

    if (!notification_is_exisit) {
      content::GetUIThreadTaskRunner({})->PostTask(
          FROM_HERE,
          base::BindOnce(&Notification::NotificationDismissed, notification_));

      if (IsDebuggingNotifications())
        LOG(INFO) << "Notification dismissed";
    }
  }

  return S_OK;
}

IFACEMETHODIMP ToastEventHandler::Invoke(
    ABI::Windows::UI::Notifications::IToastNotification* sender,
    ABI::Windows::UI::Notifications::IToastFailedEventArgs* e) {
  // feat: Upgrade for get_Failed callback
  HRESULT error;
  e->get_ErrorCode(&error);
  std::string errorMessage =
      "Notification failed. HRESULT:" + std::to_string(error);
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&Notification::NotificationFailed,
                                notification_, errorMessage));
  if (IsDebuggingNotifications())
    LOG(INFO) << errorMessage;

  return S_OK;
}

struct CoTaskMemStringTraits {
  typedef PWSTR Type;

  inline static bool Close(_In_ Type h) throw() {
    ::CoTaskMemFree(h);
    return true;
  }

  inline static Type GetInvalidValue() throw() { return nullptr; }
};
typedef HandleT<CoTaskMemStringTraits> CoTaskMemString;

// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wmissing-braces"
// #pragma GCC diagnostic ignored "-Wc++98-compat-extra-semi"
//  For the app to be activated from Action Center, it needs to provide a COM
//  server to be called when the notification is activated.  The CLSID of the
//  object needs t  registered with the OS via its shortcut so that it knows
//  who to call later.
class NotificationActivator final
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>,
                          INotificationActivationCallback> {
 public:
  HRESULT STDMETHODCALLTYPE
  Activate(_In_ LPCWSTR appUserModelId,
           _In_ LPCWSTR invokedArgs,
           _In_reads_(dataCount) const NOTIFICATION_USER_INPUT_DATA* data,
           ULONG dataCount) override {
    // packing user input structs into stream
    std::wstringstream json;
    // feat: Refine notification-activation for cold start
    json << L"[";  // json array open brace
    std::for_each(
        data, data + dataCount, [&](const NOTIFICATION_USER_INPUT_DATA& item) {
          json << item
               // avoid problem with last delimeter durig array-items packing
               << (--dataCount == 0 ? L"" : L",");
        });
    json << L"]";  // json array close brace
    auto* app = api::App::Get();
    if (app)
      app->Emit("notification-activation", std::wstring(invokedArgs),
                json.str());
    return S_OK;
  }
};

class NotificationActivatorFactory final
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IClassFactory> {
 public:
  HRESULT __stdcall CreateInstance(IUnknown* outer,
                                   GUID const& iid,
                                   void** result) noexcept override {
    *result = nullptr;

    if (outer) {
      return CLASS_E_NOAGGREGATION;
    }

    return Make<electron::NotificationActivator>()->QueryInterface(iid, result);
  }

  HRESULT __stdcall LockServer(BOOL) noexcept override { return S_OK; }
};
// #pragma GCC diagnostic pop

bool NotificationRegistrator::RegisterAppForNotificationSupport(
    const std::wstring& appId,
    const std::wstring& clsid,
    const std::wstring& disp) {
  if (app_path_.empty())
    return false;

  // Update registry with activator
  std::wstring key_path =
      LR"(SOFTWARE\Classes\CLSID\)" + clsid + LR"(\LocalServer32)";
  std::wstring launchStr = L"\"" + app_path_ + L"\" " + clsid;
  if (!SetRegistryKeyValue(HKEY_CURRENT_USER, key_path, L"", launchStr))
    return false;

  // Register via registry
  std::wstring subKey = LR"(SOFTWARE\Classes\AppUserModelId\)" + appId;
  // Set the display name
  if (!SetRegistryKeyValue(HKEY_CURRENT_USER, subKey, L"DisplayName",
                           !disp.empty() ? disp : appId))
    return false;
  // Set up custom activator
  if (!SetRegistryKeyValue(HKEY_CURRENT_USER, subKey, L"CustomActivator",
                           clsid))
    return false;
  // feat: Allow App icon into Action center and System Notifications
  // settings
  if (ExtractAppIconToTempFile(clsid) &&
      // Some incformation about IconUri can be found on the link
      // https://docs.microsoft.com/en-us/windows/apps/design/shell/tiles-and-notifications/send-local-toast-other-apps
      !SetRegistryKeyValue(HKEY_CURRENT_USER, subKey, L"IconUri", ico_path_))
    // there is no nesecity to fail register at all in case if icon was not set
    DeleteFile(ico_path_.c_str());

  // feat: Clear notifications
  return app_id_.Reset(appId), true;
}

#pragma pack(1)
// Structs below were taken from
// https://docs.microsoft.com/en-us/previous-versions/ms997538(v=msdn.10)?redirectedfrom=MSDN
// and necessary for storing app icon stripped from executable to file
// Unfortunately well known approach with usage OleSavePicture gives low quality
// of ico data that is why it is necessary to store icon by ourself
typedef struct {
  BYTE bWidth;          // Width, in pixels, of the image
  BYTE bHeight;         // Height, in pixels, of the image
  BYTE bColorCount;     // Number of colors in image (0 if >=8bpp)
  BYTE bReserved;       // Reserved ( must be 0)
  WORD wPlanes;         // Color Planes
  WORD wBitCount;       // Bits per pixel
  DWORD dwBytesInRes;   // How many bytes in this resource?
  DWORD dwImageOffset;  // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct {
  WORD idReserved;            // Reserved (must be 0)
  WORD idType;                // Resource Type (1 for icons)
  WORD idCount;               // How many images?
  ICONDIRENTRY idEntries[1];  // An entry for each image (idCount of 'em)
} ICONDIR, *LPICONDIR;
#pragma pack()
// ExternalModule should call LocalFree to dealocate memory
PBITMAPINFO GetBITMAPInfo(HBITMAP hBitmap) {
  BITMAP bmp = {0};
  if (!GetObject(hBitmap, sizeof(BITMAP), &bmp))
    return NULL;

  WORD cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
  WORD biClrUsed = cClrBits < 24 ? (1 << cClrBits) : 0;
  PBITMAPINFO pbmi = (PBITMAPINFO)LocalAlloc(
      LPTR, sizeof(BITMAPINFOHEADER) +
                // There is no RGBQUAD array for these formats: 24-bit-per-pixel
                // or 32-bit-per-pixel
                sizeof(RGBQUAD) * biClrUsed);

  BITMAPINFOHEADER& hdr = pbmi->bmiHeader;

  hdr.biSize = sizeof(BITMAPINFOHEADER);
  hdr.biWidth = bmp.bmWidth;
  hdr.biHeight = bmp.bmHeight;
  hdr.biPlanes = bmp.bmPlanes;
  hdr.biBitCount = bmp.bmBitsPixel;
  hdr.biCompression = BI_RGB;
  // Next metrics will be clarified by GetDIBits with call lpvBits = NULL
  // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-getdibits
  hdr.biSizeImage = bmp.bmWidthBytes * bmp.bmHeight;
  hdr.biClrUsed = biClrUsed;

  return pbmi;
}

bool GetBitmapBits(std::vector<BYTE>* lpBmpBits,
                   HBITMAP hBitmap,
                   PBITMAPINFO pBitmapInfo) {
  HDC hDC = GetDC(NULL);  // Get Desktop device context
  // Make compatible device context
  HDC pDC = CreateCompatibleDC(hDC);
  if (pDC) {
    int nLines = 0;
    // This call is necessary to clarify metrics biSizeImage and biClrUsed
    if (nLines =
            GetDIBits(pDC, hBitmap, 0, (WORD)pBitmapInfo->bmiHeader.biHeight,
                      NULL, pBitmapInfo, DIB_RGB_COLORS),
        !nLines)
      return false;

    lpBmpBits->resize(pBitmapInfo->bmiHeader.biSizeImage);

    // This call is necessary to copy DIB bits into bmpBits vector
    if (nLines =
            GetDIBits(pDC, hBitmap, 0, (WORD)pBitmapInfo->bmiHeader.biHeight,
                      lpBmpBits->data(), pBitmapInfo, DIB_RGB_COLORS),
        !nLines)
      return false;
  }
  return DeleteDC(pDC), true;
}

bool NotificationRegistrator::ExtractAppIconToTempFile(
    const std::wstring& file_name) {
  if (tmp_path_.empty())
    return false;

  SHFILEINFO sfi = {0};
  if (FAILED(SHGetFileInfo(app_path_.c_str(), FILE_ATTRIBUTE_NORMAL, &sfi,
                           sizeof(sfi), SHGFI_ICON)))
    return false;

  ICONINFO iconInfo = {0};
  if (!GetIconInfo(sfi.hIcon, &iconInfo))
    return false;

  // Information about color DIB into icon
  std::unique_ptr<BITMAPINFO, void (*)(PBITMAPINFO)> bmColorInfo{
      GetBITMAPInfo(iconInfo.hbmColor),
      [](PBITMAPINFO p) { LocalFree(p); }  // custom deleter
  };
  if (!bmColorInfo)
    return false;

  // Information about color DIB bits icon
  std::vector<BYTE> bmpColorBits;
  if (!GetBitmapBits(&bmpColorBits, iconInfo.hbmColor, bmColorInfo.get()))
    return false;

  // Information about mask DIB into icon
  std::unique_ptr<BITMAPINFO, void (*)(PBITMAPINFO)> bmMaskInfo{
      GetBITMAPInfo(iconInfo.hbmMask),
      [](PBITMAPINFO p) { LocalFree(p); }  // custom deleter
  };
  if (!bmMaskInfo)
    return false;

  // Information about mask DIB bits icon
  std::vector<BYTE> bmpMaskBits;
  if (!GetBitmapBits(&bmpMaskBits, iconInfo.hbmMask, bmMaskInfo.get()))
    return false;

  ico_path_ = tmp_path_ + file_name + L".ico";
  // Now we have all necessary metrics to store icon into file
  ScopedHandle ico_file_(
      CreateFileW(ico_path_.c_str(), GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                  FILE_ATTRIBUTE_NORMAL, NULL));

  if (ico_file_.Get() == INVALID_HANDLE_VALUE)
    return false;

  // Filling icon file metrics
  ICONDIR iconDir = {0, 1, 1};
  ICONDIRENTRY& entry = iconDir.idEntries[0];

  entry.bWidth = bmColorInfo->bmiHeader.biWidth;
  entry.bHeight = bmColorInfo->bmiHeader.biHeight;
  entry.bColorCount = bmColorInfo->bmiHeader.biClrUsed;
  entry.bReserved = 0;
  entry.wPlanes = bmColorInfo->bmiHeader.biPlanes;
  entry.wBitCount = bmColorInfo->bmiHeader.biBitCount;
  entry.dwBytesInRes =
      bmColorInfo->bmiHeader.biSizeImage + bmMaskInfo->bmiHeader.biSizeImage;
  entry.dwImageOffset = sizeof(ICONDIR);

  if (!WriteFile(ico_file_.Get(), &iconDir, sizeof(ICONDIR), NULL, NULL))
    return false;

  // Stripped from original MSDN sample mentioned into
  // https://docs.microsoft.com/en-us/previous-versions/ms997538(v=msdn.10)?redirectedfrom=MSDN
  // quote : "Complete code can be found in the Icons.C module of IconPro, in a
  // function named ReadIconFromICOFile" ReadIconFromICOFile : Icons are stored
  // in funky format where height is doubled - account for it
  bmColorInfo->bmiHeader.biHeight *= 2;

  if (!WriteFile(ico_file_.Get(), bmColorInfo.get(), sizeof(BITMAPINFO), NULL,
                 NULL))
    return false;

  if (!WriteFile(ico_file_.Get(), bmpColorBits.data(),
                 bmColorInfo->bmiHeader.biSizeImage, NULL, NULL))
    return false;

  if (!WriteFile(ico_file_.Get(), bmpMaskBits.data(),
                 bmMaskInfo->bmiHeader.biSizeImage, NULL, NULL))
    return false;

  return true;
}

}  // namespace electron
