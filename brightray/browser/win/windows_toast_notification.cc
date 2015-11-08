// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon <jason.poon@microsoft.com>. All rights reserved.
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of this code
// and released it as MIT to the world.

#include "windows_toast_notification.h"
#include "content/public/browser/desktop_notification_delegate.h"

using namespace WinToasts;
using namespace Microsoft::WRL;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;

#define BREAK_IF_BAD(hr)    if(!SUCCEEDED(hr)) break;

namespace WinToasts {

// Initialize Windows Runtime
static HRESULT init = Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);

WindowsToastNotification::WindowsToastNotification(const char* appName, content::DesktopNotificationDelegate* delegate)
{
    HSTRING toastNotifMgrStr = NULL;
    HSTRING appId = NULL;

    HRESULT hr = CreateHString(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager, &toastNotifMgrStr);

    hr = Windows::Foundation::GetActivationFactory(toastNotifMgrStr, &m_toastManager);

    WCHAR wAppName[MAX_PATH];
    swprintf(wAppName, ARRAYSIZE(wAppName), L"%S", appName);
    hr = CreateHString(wAppName, &appId);

    m_toastManager->CreateToastNotifierWithId(appId, &m_toastNotifier);

    if (toastNotifMgrStr != NULL) {
        WindowsDeleteString(toastNotifMgrStr);
    }

    if (appId != NULL) {
        WindowsDeleteString(appId);
    }

    m_eventHandler = NULL;
    n_delegate = delegate;
}

WindowsToastNotification::~WindowsToastNotification()
{
    if (m_eventHandler) {
        delete m_eventHandler;
    }
    
    if (n_delegate) {
        delete n_delegate;
    }
}

void WindowsToastNotification::ShowNotification(const WCHAR* title, const WCHAR* msg, std::string iconPath, ComPtr<IToastNotification>& toast)
{        
    HRESULT hr;
    HSTRING toastNotifStr = NULL;

    do {         
        ComPtr<IXmlDocument> toastXml;
        hr = GetToastXml(m_toastManager.Get(), title, msg, iconPath, &toastXml);
        BREAK_IF_BAD(hr);

        hr = CreateHString(RuntimeClass_Windows_UI_Notifications_ToastNotification, &toastNotifStr);
        BREAK_IF_BAD(hr);

        ComPtr<IToastNotificationFactory> toastFactory;
        hr = Windows::Foundation::GetActivationFactory(toastNotifStr, &toastFactory);
        BREAK_IF_BAD(hr);

        hr = toastFactory->CreateToastNotification(toastXml.Get(), &toast);
        BREAK_IF_BAD(hr);

        hr = SetupCallbacks(toast.Get());
        BREAK_IF_BAD(hr);

        hr = m_toastNotifier->Show(toast.Get());
        BREAK_IF_BAD(hr);

        n_delegate->NotificationDisplayed();
    } while (FALSE);

    if (toastNotifStr != NULL) {
        WindowsDeleteString(toastNotifStr);
    }
}

void WindowsToastNotification::DismissNotification(ComPtr<IToastNotification> toast)
{
    m_toastNotifier->Hide(toast.Get());
}

void WindowsToastNotification::NotificationClicked()
{
    delete this;
}

void WindowsToastNotification::NotificationDismissed()
{
    delete this;
}

HRESULT WindowsToastNotification::GetToastXml(
    IToastNotificationManagerStatics* toastManager,
    const WCHAR* title,
    const WCHAR* msg,
    std::string iconPath,
    IXmlDocument** toastXml) {
 
    HRESULT hr;
    ToastTemplateType templateType;
    if (title == NULL || msg == NULL) {
        // Single line toast
        templateType = iconPath.length() == 0 ? ToastTemplateType_ToastText01 : ToastTemplateType_ToastImageAndText01;
        hr = m_toastManager->GetTemplateContent(templateType, toastXml);
        if (SUCCEEDED(hr)) {
            const WCHAR* text = title != NULL ? title : msg;
            hr = SetXmlText(*toastXml, text);
        }
    } else {
        // Title and body toast
        templateType = iconPath.length() == 0 ? ToastTemplateType_ToastText02 : ToastTemplateType_ToastImageAndText02;
        hr = toastManager->GetTemplateContent(templateType, toastXml);
        if (SUCCEEDED(hr)) {
            hr = SetXmlText(*toastXml, title, msg);
        }
    }

    if (iconPath.length() != 0 && SUCCEEDED(hr)) {
        // Toast has image
        if (SUCCEEDED(hr)) {
            hr = SetXmlImage(*toastXml, iconPath);
        }

        // Don't stop a notification from showing just because an image couldn't be displayed. By default the app icon will be shown.
        hr = S_OK;
    }

    return hr;
}

HRESULT WindowsToastNotification::SetXmlText(IXmlDocument* doc, const WCHAR* text)
{
    HSTRING tag = NULL;

    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = GetTextNodeList(&tag, doc, &nodeList, 1);
    do {
        BREAK_IF_BAD(hr);

        ComPtr<IXmlNode> node;
        hr = nodeList->Item(0, &node);
        BREAK_IF_BAD(hr);

        hr = AppendTextToXml(doc, node.Get(), text);
    } while (FALSE);

    if (tag != NULL) {
        WindowsDeleteString(tag);
    }

    return hr;
}

HRESULT WindowsToastNotification::SetXmlText(IXmlDocument* doc, const WCHAR* title, const WCHAR* body)
{
    HSTRING tag = NULL;
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = GetTextNodeList(&tag, doc, &nodeList, 2);
    do {
        BREAK_IF_BAD(hr);

        ComPtr<IXmlNode> node;
        hr = nodeList->Item(0, &node);
        BREAK_IF_BAD(hr);

        hr = AppendTextToXml(doc, node.Get(), title);
        BREAK_IF_BAD(hr);

        hr = nodeList->Item(1, &node);
        BREAK_IF_BAD(hr);

        hr = AppendTextToXml(doc, node.Get(), body);
    } while (FALSE);

    if (tag != NULL) {
        WindowsDeleteString(tag);
    }

    return hr;
}

HRESULT WindowsToastNotification::SetXmlImage(IXmlDocument* doc, std::string iconPath)
{
    HSTRING tag = NULL;
    HSTRING src = NULL;
    HSTRING imgPath = NULL;
    HRESULT hr = CreateHString(L"image", &tag);

    do {
        BREAK_IF_BAD(hr);

        ComPtr<IXmlNodeList> nodeList;
        hr = doc->GetElementsByTagName(tag, &nodeList);
        BREAK_IF_BAD(hr);

        ComPtr<IXmlNode> imageNode;
        hr = nodeList->Item(0, &imageNode);
        BREAK_IF_BAD(hr);

        ComPtr<IXmlNamedNodeMap> attrs;
        hr = imageNode->get_Attributes(&attrs);
        BREAK_IF_BAD(hr);

        hr = CreateHString(L"src", &src);
        BREAK_IF_BAD(hr);

        ComPtr<IXmlNode> srcAttr;
        hr = attrs->GetNamedItem(src, &srcAttr);
        BREAK_IF_BAD(hr);

        WCHAR xmlPath[MAX_PATH];
        swprintf(xmlPath, ARRAYSIZE(xmlPath), L"%S", iconPath);
        hr = CreateHString(xmlPath, &imgPath);
        BREAK_IF_BAD(hr);

        ComPtr<IXmlText> srcText;
        hr = doc->CreateTextNode(imgPath, &srcText);
        BREAK_IF_BAD(hr);

        ComPtr<IXmlNode> srcNode;
        hr = srcText.As(&srcNode);
        BREAK_IF_BAD(hr);

        ComPtr<IXmlNode> childNode;
        hr = srcAttr->AppendChild(srcNode.Get(), &childNode);
    } while (FALSE);

    if (tag != NULL) {
        WindowsDeleteString(tag);
    }
    if (src != NULL) {
        WindowsDeleteString(src);
    }
    if (imgPath != NULL) {
        WindowsDeleteString(imgPath);
    }

    return hr;
}

HRESULT WindowsToastNotification::GetTextNodeList(HSTRING* tag, IXmlDocument* doc, IXmlNodeList** nodeList, UINT32 reqLength)
{
    HRESULT hr = CreateHString(L"text", tag);
    do{
        BREAK_IF_BAD(hr);

        hr = doc->GetElementsByTagName(*tag, nodeList);
        BREAK_IF_BAD(hr);

        UINT32 nodeLength;
        hr = (*nodeList)->get_Length(&nodeLength);
        BREAK_IF_BAD(hr);

        if (nodeLength < reqLength) {
            hr = E_INVALIDARG;
        }
    } while (FALSE);

    if (!SUCCEEDED(hr)) {
        // Allow the caller to delete this string on success
        WindowsDeleteString(*tag);
    }

    return hr;
}

HRESULT WindowsToastNotification::AppendTextToXml(IXmlDocument* doc, IXmlNode* node, const WCHAR* text)
{
    HSTRING str = NULL;
    HRESULT hr = CreateHString(text, &str);
    do {
        BREAK_IF_BAD(hr);

        ComPtr<IXmlText> xmlText;
        hr = doc->CreateTextNode(str, &xmlText);
        BREAK_IF_BAD(hr);

        ComPtr<IXmlNode> textNode;
        hr = xmlText.As(&textNode);
        BREAK_IF_BAD(hr);

        ComPtr<IXmlNode> appendNode;
        hr = node->AppendChild(textNode.Get(), &appendNode);
    } while (FALSE);

    if (str != NULL) {
        WindowsDeleteString(str);
    }

    return hr;
}

HRESULT WindowsToastNotification::SetupCallbacks(IToastNotification* toast)
{
    EventRegistrationToken activatedToken, dismissedToken;
    m_eventHandler = new ToastEventHandler(this, n_delegate);
    ComPtr<ToastEventHandler> eventHandler(m_eventHandler);
    HRESULT hr = toast->add_Activated(eventHandler.Get(), &activatedToken);
    
    if (SUCCEEDED(hr)) {
        hr = toast->add_Dismissed(eventHandler.Get(), &dismissedToken);
    }

    return hr;
}

HRESULT WindowsToastNotification::CreateHString(const WCHAR* source, HSTRING* dest)
{
    if (source == NULL || dest == NULL) {
        return E_INVALIDARG;
    }

    HRESULT hr = WindowsCreateString(source, wcslen(source), dest);
    return hr;
}

/*
/ Toast Event Handler
*/
ToastEventHandler::ToastEventHandler(WindowsToastNotification* notification, content::DesktopNotificationDelegate* delegate)
{
    m_ref = 1;
    m_notification = notification;
    n_delegate = delegate;
}

ToastEventHandler::~ToastEventHandler()
{
    // Empty
}

IFACEMETHODIMP ToastEventHandler::Invoke(IToastNotification* sender, IInspectable* args)
{
    // Notification "activated" (clicked)
    n_delegate->NotificationClick();
    
    if (m_notification != NULL) {
        m_notification->NotificationClicked();
    }
    
    return S_OK;
}

IFACEMETHODIMP ToastEventHandler::Invoke(IToastNotification* sender, IToastDismissedEventArgs* e)
{   
    // Notification dismissed
    n_delegate->NotificationClosed();

    if (m_notification != NULL) {
        m_notification->NotificationDismissed();
        
    }
    
    return S_OK;
}

} //namespace