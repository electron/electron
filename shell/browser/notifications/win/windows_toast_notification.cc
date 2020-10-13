// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon
// <jason.poon@microsoft.com>. All rights reserved.
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith
// <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of
// this code
// and released it as MIT to the world.

#include "shell/browser/notifications/win/windows_toast_notification.h"

#include <shlobj.h>
#include <vector>

#include "base/environment.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/win/notification_presenter_win.h"
#include "shell/browser/win/scoped_hstring.h"
#include "shell/common/application_info.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

using ABI::Windows::Data::Xml::Dom::IXmlAttribute;
using ABI::Windows::Data::Xml::Dom::IXmlDocument;
using ABI::Windows::Data::Xml::Dom::IXmlElement;
using ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap;
using ABI::Windows::Data::Xml::Dom::IXmlNode;
using ABI::Windows::Data::Xml::Dom::IXmlNodeList;
using ABI::Windows::Data::Xml::Dom::IXmlText;

namespace electron {

namespace {

bool IsDebuggingNotifications() {
  return base::Environment::Create()->HasVar("ELECTRON_DEBUG_NOTIFICATIONS");
}

}  // namespace

// static
ComPtr<ABI::Windows::UI::Notifications::IToastNotificationManagerStatics>
    WindowsToastNotification::toast_manager_;

// static
ComPtr<ABI::Windows::UI::Notifications::IToastNotifier>
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
    Dismiss();
  }
}

void WindowsToastNotification::Show(const NotificationOptions& options) {
  auto* presenter_win = static_cast<NotificationPresenterWin*>(presenter());
  std::wstring icon_path =
      presenter_win->SaveIconToFilesystem(options.icon, options.icon_url);

  ComPtr<IXmlDocument> toast_xml;
  if (FAILED(GetToastXml(toast_manager_.Get(), options.title, options.msg,
                         icon_path, options.timeout_type, options.silent,
                         &toast_xml))) {
    NotificationFailed();
    return;
  }

  ScopedHString toast_str(
      RuntimeClass_Windows_UI_Notifications_ToastNotification);
  if (!toast_str.success()) {
    NotificationFailed();
    return;
  }

  ComPtr<ABI::Windows::UI::Notifications::IToastNotificationFactory>
      toast_factory;
  if (FAILED(Windows::Foundation::GetActivationFactory(toast_str,
                                                       &toast_factory))) {
    NotificationFailed();
    return;
  }

  if (FAILED(toast_factory->CreateToastNotification(toast_xml.Get(),
                                                    &toast_notification_))) {
    NotificationFailed();
    return;
  }

  if (!SetupCallbacks(toast_notification_.Get())) {
    NotificationFailed();
    return;
  }

  if (FAILED(toast_notifier_->Show(toast_notification_.Get()))) {
    NotificationFailed();
    return;
  }

  if (IsDebuggingNotifications())
    LOG(INFO) << "Notification created";

  if (delegate())
    delegate()->NotificationDisplayed();
}

void WindowsToastNotification::Dismiss() {
  if (IsDebuggingNotifications())
    LOG(INFO) << "Hiding notification";
  toast_notifier_->Hide(toast_notification_.Get());
}

bool WindowsToastNotification::GetToastXml(
    ABI::Windows::UI::Notifications::IToastNotificationManagerStatics*
        toastManager,
    const std::wstring& title,
    const std::wstring& msg,
    const std::wstring& icon_path,
    const std::wstring& timeout_type,
    bool silent,
    IXmlDocument** toast_xml) {
  ABI::Windows::UI::Notifications::ToastTemplateType template_type;
  if (title.empty() || msg.empty()) {
    // Single line toast.
    template_type =
        icon_path.empty()
            ? ABI::Windows::UI::Notifications::ToastTemplateType_ToastText01
            : ABI::Windows::UI::Notifications::
                  ToastTemplateType_ToastImageAndText01;
    if (FAILED(toast_manager_->GetTemplateContent(template_type, toast_xml)))
      return false;
    if (!SetXmlText(*toast_xml, title.empty() ? msg : title))
      return false;
  } else {
    // Title and body toast.
    template_type =
        icon_path.empty()
            ? ABI::Windows::UI::Notifications::ToastTemplateType_ToastText02
            : ABI::Windows::UI::Notifications::
                  ToastTemplateType_ToastImageAndText02;
    if (FAILED(toastManager->GetTemplateContent(template_type, toast_xml))) {
      if (IsDebuggingNotifications())
        LOG(INFO) << "Fetching XML template failed";
      return false;
    }

    if (!SetXmlText(*toast_xml, title, msg)) {
      if (IsDebuggingNotifications())
        LOG(INFO) << "Setting text fields on template failed";
      return false;
    }
  }

  // Configure the toast's timeout settings
  if (timeout_type == base::ASCIIToUTF16("never")) {
    if (FAILED(SetXmlScenarioReminder(*toast_xml))) {
      if (IsDebuggingNotifications())
        LOG(INFO) << "Setting \"scenario\" option on notification failed";
      return false;
    }
  }

  // Configure the toast's notification sound
  if (silent) {
    if (FAILED(SetXmlAudioSilent(*toast_xml))) {
      if (IsDebuggingNotifications()) {
        LOG(INFO) << "Setting \"silent\" option on notification failed";
      }

      return false;
    }
  }

  // Configure the toast's image
  if (!icon_path.empty())
    return SetXmlImage(*toast_xml, icon_path);

  return true;
}

bool WindowsToastNotification::SetXmlScenarioReminder(IXmlDocument* doc) {
  ScopedHString tag(L"toast");
  if (!tag.success())
    return false;

  ComPtr<IXmlNodeList> node_list;
  if (FAILED(doc->GetElementsByTagName(tag, &node_list)))
    return false;

  // Check that root "toast" node exists
  ComPtr<IXmlNode> root;
  if (FAILED(node_list->Item(0, &root)))
    return false;

  // get attributes of root "toast" node
  ComPtr<IXmlNamedNodeMap> toast_attributes;
  if (FAILED(root->get_Attributes(&toast_attributes)))
    return false;

  ComPtr<IXmlAttribute> scenario_attribute;
  ScopedHString scenario_str(L"scenario");
  if (FAILED(doc->CreateAttribute(scenario_str, &scenario_attribute)))
    return false;

  ComPtr<IXmlNode> scenario_attribute_node;
  if (FAILED(scenario_attribute.As(&scenario_attribute_node)))
    return false;

  ScopedHString scenario_value(L"reminder");
  if (!scenario_value.success())
    return false;

  ComPtr<IXmlText> scenario_text;
  if (FAILED(doc->CreateTextNode(scenario_value, &scenario_text)))
    return false;

  ComPtr<IXmlNode> scenario_node;
  if (FAILED(scenario_text.As(&scenario_node)))
    return false;

  ComPtr<IXmlNode> scenario_backup_node;
  if (FAILED(scenario_attribute_node->AppendChild(scenario_node.Get(),
                                                  &scenario_backup_node)))
    return false;

  ComPtr<IXmlNode> scenario_attribute_pnode;
  if (FAILED(toast_attributes.Get()->SetNamedItem(scenario_attribute_node.Get(),
                                                  &scenario_attribute_pnode)))
    return false;

  // Create "actions" wrapper
  ComPtr<IXmlElement> actions_wrapper_element;
  ScopedHString actions_wrapper_str(L"actions");
  if (FAILED(doc->CreateElement(actions_wrapper_str, &actions_wrapper_element)))
    return false;

  ComPtr<IXmlNode> actions_wrapper_node_tmp;
  if (FAILED(actions_wrapper_element.As(&actions_wrapper_node_tmp)))
    return false;

  // Append actions wrapper node to toast xml
  ComPtr<IXmlNode> actions_wrapper_node;
  if (FAILED(root->AppendChild(actions_wrapper_node_tmp.Get(),
                               &actions_wrapper_node)))
    return false;

  ComPtr<IXmlNamedNodeMap> attributes_actions_wrapper;
  if (FAILED(actions_wrapper_node->get_Attributes(&attributes_actions_wrapper)))
    return false;

  // Add a "Dismiss" button
  // Create "action" tag
  ComPtr<IXmlElement> action_element;
  ScopedHString action_str(L"action");
  if (FAILED(doc->CreateElement(action_str, &action_element)))
    return false;

  ComPtr<IXmlNode> action_node_tmp;
  if (FAILED(action_element.As(&action_node_tmp)))
    return false;

  // Append action node to actions wrapper in toast xml
  ComPtr<IXmlNode> action_node;
  if (FAILED(actions_wrapper_node->AppendChild(action_node_tmp.Get(),
                                               &action_node)))
    return false;

  // Setup attributes for action
  ComPtr<IXmlNamedNodeMap> action_attributes;
  if (FAILED(action_node->get_Attributes(&action_attributes)))
    return false;

  // Create activationType attribute
  ComPtr<IXmlAttribute> activation_type_attribute;
  ScopedHString activation_type_str(L"activationType");
  if (FAILED(doc->CreateAttribute(activation_type_str,
                                  &activation_type_attribute)))
    return false;

  ComPtr<IXmlNode> activation_type_attribute_node;
  if (FAILED(activation_type_attribute.As(&activation_type_attribute_node)))
    return false;

  // Set activationType attribute to system
  ScopedHString activation_type_value(L"system");
  if (!activation_type_value.success())
    return E_FAIL;

  ComPtr<IXmlText> activation_type_text;
  if (FAILED(doc->CreateTextNode(activation_type_value, &activation_type_text)))
    return false;

  ComPtr<IXmlNode> activation_type_node;
  if (FAILED(activation_type_text.As(&activation_type_node)))
    return false;

  ComPtr<IXmlNode> activation_type_backup_node;
  if (FAILED(activation_type_attribute_node->AppendChild(
          activation_type_node.Get(), &activation_type_backup_node)))
    return false;

  // Add activation type to the action attributes
  ComPtr<IXmlNode> activation_type_attribute_pnode;
  if (FAILED(action_attributes.Get()->SetNamedItem(
          activation_type_attribute_node.Get(),
          &activation_type_attribute_pnode)))
    return false;

  // Create arguments attribute
  ComPtr<IXmlAttribute> arguments_attribute;
  ScopedHString arguments_str(L"arguments");
  if (FAILED(doc->CreateAttribute(arguments_str, &arguments_attribute)))
    return false;

  ComPtr<IXmlNode> arguments_attribute_node;
  if (FAILED(arguments_attribute.As(&arguments_attribute_node)))
    return false;

  // Set arguments attribute to dismiss
  ScopedHString arguments_value(L"dismiss");
  if (!arguments_value.success())
    return E_FAIL;

  ComPtr<IXmlText> arguments_text;
  if (FAILED(doc->CreateTextNode(arguments_value, &arguments_text)))
    return false;

  ComPtr<IXmlNode> arguments_node;
  if (FAILED(arguments_text.As(&arguments_node)))
    return false;

  ComPtr<IXmlNode> arguments_backup_node;
  if (FAILED(arguments_attribute_node->AppendChild(arguments_node.Get(),
                                                   &arguments_backup_node)))
    return false;

  // Add arguments to the action attributes
  ComPtr<IXmlNode> arguments_attribute_pnode;
  if (FAILED(action_attributes.Get()->SetNamedItem(
          arguments_attribute_node.Get(), &arguments_attribute_pnode)))
    return false;

  // Create content attribute
  ComPtr<IXmlAttribute> content_attribute;
  ScopedHString content_str(L"content");
  if (FAILED(doc->CreateAttribute(content_str, &content_attribute)))
    return false;

  ComPtr<IXmlNode> content_attribute_node;
  if (FAILED(content_attribute.As(&content_attribute_node)))
    return false;

  // Set content attribute to Dismiss
  ScopedHString content_value(l10n_util::GetStringUTF16(IDS_APP_CLOSE));
  if (!content_value.success())
    return E_FAIL;

  ComPtr<IXmlText> content_text;
  if (FAILED(doc->CreateTextNode(content_value, &content_text)))
    return false;

  ComPtr<IXmlNode> content_node;
  if (FAILED(content_text.As(&content_node)))
    return false;

  ComPtr<IXmlNode> content_backup_node;
  if (FAILED(content_attribute_node->AppendChild(content_node.Get(),
                                                 &content_backup_node)))
    return false;

  // Add content to the action attributes
  ComPtr<IXmlNode> content_attribute_pnode;
  return SUCCEEDED(action_attributes.Get()->SetNamedItem(
      content_attribute_node.Get(), &content_attribute_pnode));
}

bool WindowsToastNotification::SetXmlAudioSilent(IXmlDocument* doc) {
  ScopedHString tag(L"toast");
  if (!tag.success())
    return false;

  ComPtr<IXmlNodeList> node_list;
  if (FAILED(doc->GetElementsByTagName(tag, &node_list)))
    return false;

  ComPtr<IXmlNode> root;
  if (FAILED(node_list->Item(0, &root)))
    return false;

  ComPtr<IXmlElement> audio_element;
  ScopedHString audio_str(L"audio");
  if (FAILED(doc->CreateElement(audio_str, &audio_element)))
    return false;

  ComPtr<IXmlNode> audio_node_tmp;
  if (FAILED(audio_element.As(&audio_node_tmp)))
    return false;

  // Append audio node to toast xml
  ComPtr<IXmlNode> audio_node;
  if (FAILED(root->AppendChild(audio_node_tmp.Get(), &audio_node)))
    return false;

  // Create silent attribute
  ComPtr<IXmlNamedNodeMap> attributes;
  if (FAILED(audio_node->get_Attributes(&attributes)))
    return false;

  ComPtr<IXmlAttribute> silent_attribute;
  ScopedHString silent_str(L"silent");
  if (FAILED(doc->CreateAttribute(silent_str, &silent_attribute)))
    return false;

  ComPtr<IXmlNode> silent_attribute_node;
  if (FAILED(silent_attribute.As(&silent_attribute_node)))
    return false;

  // Set silent attribute to true
  ScopedHString silent_value(L"true");
  if (!silent_value.success())
    return false;

  ComPtr<IXmlText> silent_text;
  if (FAILED(doc->CreateTextNode(silent_value, &silent_text)))
    return false;

  ComPtr<IXmlNode> silent_node;
  if (FAILED(silent_text.As(&silent_node)))
    return false;

  ComPtr<IXmlNode> child_node;
  if (FAILED(
          silent_attribute_node->AppendChild(silent_node.Get(), &child_node)))
    return false;

  ComPtr<IXmlNode> silent_attribute_pnode;
  return SUCCEEDED(attributes.Get()->SetNamedItem(silent_attribute_node.Get(),
                                                  &silent_attribute_pnode));
}

bool WindowsToastNotification::SetXmlText(IXmlDocument* doc,
                                          const std::wstring& text) {
  ScopedHString tag;
  ComPtr<IXmlNodeList> node_list;
  if (!GetTextNodeList(&tag, doc, &node_list, 1))
    return false;

  ComPtr<IXmlNode> node;
  if (FAILED(node_list->Item(0, &node)))
    return false;

  return AppendTextToXml(doc, node.Get(), text);
}

bool WindowsToastNotification::SetXmlText(IXmlDocument* doc,
                                          const std::wstring& title,
                                          const std::wstring& body) {
  ScopedHString tag;
  ComPtr<IXmlNodeList> node_list;
  if (!GetTextNodeList(&tag, doc, &node_list, 2))
    return false;

  ComPtr<IXmlNode> node;
  if (FAILED(node_list->Item(0, &node)))
    return false;

  if (!AppendTextToXml(doc, node.Get(), title))
    return false;

  if (FAILED(node_list->Item(1, &node)))
    return false;

  return AppendTextToXml(doc, node.Get(), body);
}

bool WindowsToastNotification::SetXmlImage(IXmlDocument* doc,
                                           const std::wstring& icon_path) {
  ScopedHString tag(L"image");
  if (!tag.success())
    return false;

  ComPtr<IXmlNodeList> node_list;
  if (FAILED(doc->GetElementsByTagName(tag, &node_list)))
    return false;

  ComPtr<IXmlNode> image_node;
  if (FAILED(node_list->Item(0, &image_node)))
    return false;

  ComPtr<IXmlNamedNodeMap> attrs;
  if (FAILED(image_node->get_Attributes(&attrs)))
    return false;

  ScopedHString src(L"src");
  if (!src.success())
    return false;

  ComPtr<IXmlNode> src_attr;
  if (FAILED(attrs->GetNamedItem(src, &src_attr)))
    return false;

  ScopedHString img_path(icon_path.c_str());
  if (!img_path.success())
    return false;

  ComPtr<IXmlText> src_text;
  if (FAILED(doc->CreateTextNode(img_path, &src_text)))
    return false;

  ComPtr<IXmlNode> src_node;
  if (FAILED(src_text.As(&src_node)))
    return false;

  ComPtr<IXmlNode> child_node;
  return SUCCEEDED(src_attr->AppendChild(src_node.Get(), &child_node));
}

bool WindowsToastNotification::GetTextNodeList(ScopedHString* tag,
                                               IXmlDocument* doc,
                                               IXmlNodeList** node_list,
                                               uint32_t req_length) {
  tag->Reset(L"text");
  if (!tag->success())
    return false;

  if (FAILED(doc->GetElementsByTagName(*tag, node_list)))
    return false;

  uint32_t node_length;
  if (FAILED((*node_list)->get_Length(&node_length)))
    return false;

  return node_length >= req_length;
}

bool WindowsToastNotification::AppendTextToXml(IXmlDocument* doc,
                                               IXmlNode* node,
                                               const std::wstring& text) {
  ScopedHString str(text);
  if (!str.success())
    return false;

  ComPtr<IXmlText> xml_text;
  if (FAILED(doc->CreateTextNode(str, &xml_text)))
    return false;

  ComPtr<IXmlNode> text_node;
  if (FAILED(xml_text.As(&text_node)))
    return false;

  ComPtr<IXmlNode> append_node;
  return SUCCEEDED(node->AppendChild(text_node.Get(), &append_node));
}

bool WindowsToastNotification::SetupCallbacks(
    ABI::Windows::UI::Notifications::IToastNotification* toast) {
  event_handler_ = Make<ToastEventHandler>(this);
  if (FAILED(toast->add_Activated(event_handler_.Get(), &activated_token_)))
    return false;

  if (FAILED(toast->add_Dismissed(event_handler_.Get(), &dismissed_token_)))
    return false;

  return SUCCEEDED(toast->add_Failed(event_handler_.Get(), &failed_token_));
}

bool WindowsToastNotification::RemoveCallbacks(
    ABI::Windows::UI::Notifications::IToastNotification* toast) {
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

ToastEventHandler::~ToastEventHandler() {}

IFACEMETHODIMP ToastEventHandler::Invoke(
    ABI::Windows::UI::Notifications::IToastNotification* sender,
    IInspectable* args) {
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&Notification::NotificationClicked, notification_));
  if (IsDebuggingNotifications())
    LOG(INFO) << "Notification clicked";

  return S_OK;
}

IFACEMETHODIMP ToastEventHandler::Invoke(
    ABI::Windows::UI::Notifications::IToastNotification* sender,
    ABI::Windows::UI::Notifications::IToastDismissedEventArgs* e) {
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&Notification::NotificationDismissed, notification_));
  if (IsDebuggingNotifications())
    LOG(INFO) << "Notification dismissed";

  return S_OK;
}

IFACEMETHODIMP ToastEventHandler::Invoke(
    ABI::Windows::UI::Notifications::IToastNotification* sender,
    ABI::Windows::UI::Notifications::IToastFailedEventArgs* e) {
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&Notification::NotificationFailed, notification_));
  if (IsDebuggingNotifications())
    LOG(INFO) << "Notification failed";

  return S_OK;
}

}  // namespace electron
