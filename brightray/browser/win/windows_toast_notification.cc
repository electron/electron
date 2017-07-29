// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon
// <jason.poon@microsoft.com>. All rights reserved.
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith
// <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of
// this code
// and released it as MIT to the world.

#include "brightray/browser/win/windows_toast_notification.h"

#include <shlobj.h>
#include <sstream>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/notification_delegate.h"
#include "brightray/browser/win/notification_presenter_win.h"
#include "brightray/browser/win/scoped_hstring.h"
#include "brightray/common/application_info.h"
#include "content/public/browser/browser_thread.h"

using ABI::Windows::Data::Xml::Dom::IXmlAttribute;
using ABI::Windows::Data::Xml::Dom::IXmlDocument;
using ABI::Windows::Data::Xml::Dom::IXmlElement;
using ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap;
using ABI::Windows::Data::Xml::Dom::IXmlNode;
using ABI::Windows::Data::Xml::Dom::IXmlNodeList;
using ABI::Windows::Data::Xml::Dom::IXmlText;

namespace brightray {

namespace {

bool GetAppUserModelId(ScopedHString* app_id) {
  PWSTR current_app_id;
  if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&current_app_id))) {
    app_id->Reset(current_app_id);
    CoTaskMemFree(current_app_id);
  } else {
    app_id->Reset(base::UTF8ToUTF16(GetApplicationName()));
  }
  return app_id->success();
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

  ScopedHString app_id;
  if (!GetAppUserModelId(&app_id))
    return false;

  return SUCCEEDED(
      toast_manager_->CreateToastNotifierWithId(app_id, &toast_notifier_));
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
  auto presenter_win = static_cast<NotificationPresenterWin*>(presenter());
  std::wstring icon_path = presenter_win->SaveIconToFilesystem(
    options.icon,
    options.icon_url);

  ComPtr<IXmlDocument> toast_xml;
  if (FAILED(GetToastXml(toast_manager_.Get(), options.title, options.msg,
                         icon_path, options.silent, options.actions,
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

  if (delegate())
    delegate()->NotificationDisplayed();
}

void WindowsToastNotification::Dismiss() {
  toast_notifier_->Hide(toast_notification_.Get());
}

bool WindowsToastNotification::GetToastXml(
    ABI::Windows::UI::Notifications::IToastNotificationManagerStatics*
        toastManager,
    const std::wstring& title,
    const std::wstring& msg,
    const std::wstring& icon_path,
    bool silent,
    const std::vector<NotificationAction> actions,
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
    if (FAILED(toastManager->GetTemplateContent(template_type, toast_xml)))
      return false;
    if (!SetXmlText(*toast_xml, title, msg))
      return false;
  }

  // Configure the toast's notification sound
  if (silent) {
    if (FAILED(SetXmlAudioSilent(*toast_xml)))
      return false;
  }

  // Configure the toast's image
  if (!icon_path.empty())
    return SetXmlImage(*toast_xml, icon_path);

  if (FAILED(AddActions(*toast_xml, actions)))
    return false;

  return true;
}

bool WindowsToastNotification::AddAttribute(IXmlDocument* doc,
                                            ComPtr<IXmlNamedNodeMap> attrs,
                                            std::wstring name,
                                            std::wstring value) {
  // Create attribute
  ComPtr<IXmlAttribute> attribute;
  ScopedHString attr_str(name);
  if (FAILED(doc->CreateAttribute(attr_str, &attribute)))
    return false;

  ComPtr<IXmlNode> attribute_node;
  if (FAILED(attribute.As(&attribute_node)))
    return false;

  // Set content attribute to value
  ScopedHString attr_value(value);
  if (!attr_value.success())
    return false;

  ComPtr<IXmlText> attr_text;
  if (FAILED(doc->CreateTextNode(attr_value, &attr_text)))
    return false;

  ComPtr<IXmlNode> attr_node;
  if (FAILED(attr_text.As(&attr_node)))
    return false;

  ComPtr<IXmlNode> child_node;
  if (FAILED(
          attribute_node->AppendChild(attr_node.Get(), &child_node)))
    return false;

  ComPtr<IXmlNode> attribute_pnode;
  if (FAILED(attrs.Get()->SetNamedItem(attribute_node.Get(),
                                                  &attribute_pnode))) {
    return false;
  }

  return true;
}

bool WindowsToastNotification::AddActions(IXmlDocument* doc,
                                          const std::vector<NotificationAction>
                                            actions) {
  int buttons = 0;
  base::string16 buttonType = base::UTF8ToUTF16("button");
  for (auto action : actions) {
    if (action.type == buttonType) {
      buttons++;
    }
  }
  // If there are no buttons, let's stop right here
  if (buttons == 0)
    return true;

  // Create the "actions" element container
  ScopedHString tag(L"toast");
  if (!tag.success())
    return false;

  ComPtr<IXmlNodeList> node_list;
  if (FAILED(doc->GetElementsByTagName(tag, &node_list)))
    return false;

  ComPtr<IXmlNode> root;
  if (FAILED(node_list->Item(0, &root)))
    return false;

  ComPtr<IXmlElement> actions_element;
  ScopedHString actions_str(L"actions");
  if (FAILED(doc->CreateElement(actions_str, &actions_element)))
    return false;

  ComPtr<IXmlNode> actions_node_tmp;
  if (FAILED(actions_element.As(&actions_node_tmp)))
    return false;

  // Append actions node to toast xml
  ComPtr<IXmlNode> actions_node;
  if (FAILED(root->AppendChild(actions_node_tmp.Get(), &actions_node)))
    return false;

  for (int i = 0; i < actions.size(); i++) {
    auto action = actions[i];
    if (action.type == buttonType) {
      // Create action element
      ComPtr<IXmlElement> action_element;
      ScopedHString action_str(L"action");
      if (FAILED(doc->CreateElement(action_str, &action_element)))
        return false;

      ComPtr<IXmlNode> action_tmp;
      if (FAILED(action_element.As(&action_tmp)))
        return false;

      // Append action node to actions xml
      ComPtr<IXmlNode> action_node;
      if (FAILED(actions_node->AppendChild(action_tmp.Get(), &action_node)))
        return false;

      // Get attributes
      ComPtr<IXmlNamedNodeMap> attributes;
      if (FAILED(action_node->get_Attributes(&attributes)))
        return false;

      if (FAILED(AddAttribute(doc, attributes, L"content", action.text)))
        return false;

      std::ostringstream index;
      index << i;
      base::string16 launchString = base::UTF8ToUTF16(
        base::UTF16ToUTF8(action._protocol) + "/button?id=" + index.str());

      if (FAILED(AddAttribute(doc, attributes, L"arguments", launchString)))
        return false;

      if (FAILED(AddAttribute(doc, attributes, L"activationType", L"protocol")))
        return false;
    }
  }

  return true;
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
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&Notification::NotificationClicked, notification_));
  return S_OK;
}

IFACEMETHODIMP ToastEventHandler::Invoke(
    ABI::Windows::UI::Notifications::IToastNotification* sender,
    ABI::Windows::UI::Notifications::IToastDismissedEventArgs* e) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&Notification::NotificationDismissed, notification_));
  return S_OK;
}

IFACEMETHODIMP ToastEventHandler::Invoke(
    ABI::Windows::UI::Notifications::IToastNotification* sender,
    ABI::Windows::UI::Notifications::IToastFailedEventArgs* e) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&Notification::NotificationFailed, notification_));
  return S_OK;
}

}  // namespace brightray
