// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon <jason.poon@microsoft.com>. All rights reserved.
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of this code
// and released it as MIT to the world.

#include "browser/win/windows_toast_notification.h"

#include "base/strings/utf_string_conversions.h"
#include "browser/win/scoped_hstring.h"
#include "content/public/browser/desktop_notification_delegate.h"

using namespace ABI::Windows::Data::Xml::Dom;

namespace brightray {

WindowsToastNotification::WindowsToastNotification(
    const std::string& app_name,
    scoped_ptr<content::DesktopNotificationDelegate> delegate)
    : delegate_(delegate.Pass()),
      weak_factory_(this) {
  // If it wasn't for Windows 7, we could do this statically
  HRESULT init = Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);

  ScopedHString toast_manager_str(
      RuntimeClass_Windows_UI_Notifications_ToastNotificationManager);
  if (!toast_manager_str.success())
    return;
  HRESULT hr = Windows::Foundation::GetActivationFactory(
      toast_manager_str, &toast_manager_);
  if (FAILED(hr))
    return;

  ScopedHString app_id(base::UTF8ToUTF16(app_name).c_str());
  if (!app_id.success())
    return;

  toast_manager_->CreateToastNotifierWithId(app_id, &toast_notifier_);
}

WindowsToastNotification::~WindowsToastNotification() {
}

void WindowsToastNotification::ShowNotification(
    const std::wstring& title,
    const std::wstring& msg,
    std::string icon_path) {
  ComPtr<IXmlDocument> toast_xml;
  if(FAILED(GetToastXml(toast_manager_.Get(), title, msg, icon_path, &toast_xml)))
    return;

  ScopedHString toast_str(
      RuntimeClass_Windows_UI_Notifications_ToastNotification);
  if (!toast_str.success())
    return;

  ComPtr<ABI::Windows::UI::Notifications::IToastNotificationFactory> toast_factory;
  if (FAILED(Windows::Foundation::GetActivationFactory(toast_str,
                                                       &toast_factory)))
    return;

  if (FAILED(toast_factory->CreateToastNotification(toast_xml.Get(),
                                                    &toast_notification_)))
    return;

  if (FAILED(SetupCallbacks(toast_notification_.Get())))
    return;

  if (FAILED(toast_notifier_->Show(toast_notification_.Get())))
    return;

  delegate_->NotificationDisplayed();
}

void WindowsToastNotification::DismissNotification() {
  toast_notifier_->Hide(toast_notification_.Get());
}

void WindowsToastNotification::NotificationClicked() {
  delegate_->NotificationClick();
  delete this;
}

void WindowsToastNotification::NotificationDismissed() {
  delegate_->NotificationClosed();
  delete this;
}

void WindowsToastNotification::NotificationFailed() {
  delete this;
}

bool WindowsToastNotification::GetToastXml(
    ABI::Windows::UI::Notifications::IToastNotificationManagerStatics* toastManager,
    const std::wstring& title,
    const std::wstring& msg,
    std::string icon_path,
    IXmlDocument** toast_xml) {
  ABI::Windows::UI::Notifications::ToastTemplateType template_type;
  if (title.empty() || msg.empty()) {
    // Single line toast.
    template_type = icon_path.empty() ? ABI::Windows::UI::Notifications::ToastTemplateType_ToastText01 :
                                        ABI::Windows::UI::Notifications::ToastTemplateType_ToastImageAndText01;
    if (FAILED(toast_manager_->GetTemplateContent(template_type, toast_xml)))
      return false;
    if (!SetXmlText(*toast_xml, title.empty() ? msg : title))
      return false;
  } else {
    // Title and body toast.
    template_type = icon_path.empty() ? ABI::Windows::UI::Notifications::ToastTemplateType_ToastText02 :
                                        ABI::Windows::UI::Notifications::ToastTemplateType_ToastImageAndText02;
    if (FAILED(toastManager->GetTemplateContent(template_type, toast_xml)))
      return false;
    if (!SetXmlText(*toast_xml, title, msg))
      return false;
  }

  // Toast has image
  if (!icon_path.empty())
    return SetXmlImage(*toast_xml, icon_path);

  return true;
}

bool WindowsToastNotification::SetXmlText(
    IXmlDocument* doc, const std::wstring& text) {
  ScopedHString tag;
  ComPtr<IXmlNodeList> node_list;
  if (!GetTextNodeList(&tag, doc, &node_list, 1))
    return false;

  ComPtr<IXmlNode> node;
  if (FAILED(node_list->Item(0, &node)))
    return false;

  return AppendTextToXml(doc, node.Get(), text);
}

bool WindowsToastNotification::SetXmlText(
    IXmlDocument* doc, const std::wstring& title, const std::wstring& body) {
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

bool WindowsToastNotification::SetXmlImage(
    IXmlDocument* doc, std::string icon_path) {
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

  ScopedHString img_path(base::UTF8ToUTF16(icon_path).c_str());
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

bool WindowsToastNotification::GetTextNodeList(
    ScopedHString* tag,
    IXmlDocument* doc,
    IXmlNodeList** node_list,
    UINT32 req_length) {
  tag->Set(L"text");
  if (!tag->success())
    return false;

  if (FAILED(doc->GetElementsByTagName(*tag, node_list)))
    return false;

  UINT32 node_length;
  if (FAILED((*node_list)->get_Length(&node_length)))
    return false;

  return node_length >= req_length;
}

bool WindowsToastNotification::AppendTextToXml(
    IXmlDocument* doc, IXmlNode* node, const std::wstring& text) {
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

bool WindowsToastNotification::SetupCallbacks(ABI::Windows::UI::Notifications::IToastNotification* toast) {
  EventRegistrationToken activatedToken, dismissedToken, failedToken;
  event_handler_ = Make<ToastEventHandler>(this);
  if (FAILED(toast->add_Activated(event_handler_.Get(), &activatedToken)))
    return false;

  if (FAILED(toast->add_Dismissed(event_handler_.Get(), &dismissedToken)))
    return false;

  return SUCCEEDED(toast->add_Failed(event_handler_.Get(), &failedToken));
}

/*
/ Toast Event Handler
*/
ToastEventHandler::ToastEventHandler(WindowsToastNotification* notification)
    : notification_(notification) {
}

ToastEventHandler::~ToastEventHandler() {
}

IFACEMETHODIMP ToastEventHandler::Invoke(
    ABI::Windows::UI::Notifications::IToastNotification* sender, IInspectable* args) {
  notification_->NotificationClicked();
  return S_OK;
}

IFACEMETHODIMP ToastEventHandler::Invoke(
    ABI::Windows::UI::Notifications::IToastNotification* sender,
    ABI::Windows::UI::Notifications::IToastDismissedEventArgs* e) {
  notification_->NotificationDismissed();
  return S_OK;
}

IFACEMETHODIMP ToastEventHandler::Invoke(
    ABI::Windows::UI::Notifications::IToastNotification* sender,
    ABI::Windows::UI::Notifications::IToastFailedEventArgs* e) {
  notification_->NotificationFailed();
  return S_OK;
}

}  // namespace brightray
