// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com>. All rights reserved.
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of this code
// and released it as MIT to the world.

#include "windows_toast_notification.h"
#include <Psapi.h>
#include <stdio.h>
#include <ShObjidl.h>
#include <propvarutil.h>
#include <propkey.h>
#include <wincrypt.h>

using namespace WinToasts;
using namespace Windows::Foundation;

#define BREAK_IF_BAD(hr)    if(!SUCCEEDED(hr)) break;
#define SHORTCUT_FORMAT     "\\Microsoft\\Windows\\Start Menu\\Programs\\%s.lnk"

char WindowsToastNotification::s_appName[MAX_PATH] = {};
char WindowsToastNotification::s_tempDirPath[MAX_PATH] = {};

namespace WinToasts {

WindowsToastNotification::WindowsToastNotification(const char* appName)
{
    sprintf_s(s_appName, ARRAYSIZE(s_appName), "%s", appName);
    m_eventHandler = NULL;
    m_tempFilePath = NULL;
}

WindowsToastNotification::~WindowsToastNotification()
{
    if (m_eventHandler){
        delete m_eventHandler;
    }

    if (m_tempFilePath){
        DeleteTempImageFile();
        delete m_tempFilePath;
    }
}

/*static*/ void WindowsToastNotification::InitSystemProps(char* appName, char* tempDirPath)
{

    sprintf_s(s_tempDirPath, ARRAYSIZE(s_tempDirPath), "%s", tempDirPath);
}

void WindowsToastNotification::ShowNotification(const WCHAR* title, const WCHAR* msg, const char* img)
{        
    // Init using WRL
    Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);

    HSTRING toastNotifMgrStr = NULL;
    HSTRING appId = NULL;
    HSTRING toastNotifStr = NULL;
    
    // Create a shortcut in the user's app data folder
    // (requirement for Windows 8 to show notifications - Windows 10 is fine without it)
    // TODO: This shoudl be cleaned up
    // HRESULT hr = EnsureShortcut();

    do{         
        HRESULT hr = CreateHString(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager, &toastNotifMgrStr);
        BREAK_IF_BAD(hr);

        ComPtr<IToastNotificationManagerStatics> toastMgr;
        hr = Windows::Foundation::GetActivationFactory(toastNotifMgrStr, &toastMgr);
        BREAK_IF_BAD(hr);

        ComPtr<IXmlDocument> toastXml;
        hr = GetToastXml(toastMgr.Get(), title, msg, img, &toastXml);
        BREAK_IF_BAD(hr);

        WCHAR wAppName[MAX_PATH];
        swprintf(wAppName, ARRAYSIZE(wAppName), L"%S", s_appName);
        hr = CreateHString(wAppName, &appId);
        BREAK_IF_BAD(hr);

        ComPtr<IToastNotifier> notifier;
        toastMgr->CreateToastNotifierWithId(appId, &notifier);
        BREAK_IF_BAD(hr);

        hr = CreateHString(RuntimeClass_Windows_UI_Notifications_ToastNotification, &toastNotifStr);
        BREAK_IF_BAD(hr);

        ComPtr<IToastNotificationFactory> toastFactory;
        hr = Windows::Foundation::GetActivationFactory(toastNotifStr, &toastFactory);
        BREAK_IF_BAD(hr);

        ComPtr<IToastNotification> toast;
        hr = toastFactory->CreateToastNotification(toastXml.Get(), &toast);
        BREAK_IF_BAD(hr);

        hr = SetupCallbacks(toast.Get());
        BREAK_IF_BAD(hr);

        hr = notifier->Show(toast.Get());
        BREAK_IF_BAD(hr);

        //m_cbFn.Reset(Isolate::GetCurrent(), cbFn);

        //todo:
        //cbPtr->NotificationDisplayed();
    } while (FALSE);

    if (toastNotifMgrStr != NULL){
        WindowsDeleteString(toastNotifMgrStr);
    }

    if (appId != NULL){
        WindowsDeleteString(appId);
    }

    if (toastNotifStr != NULL){
        WindowsDeleteString(toastNotifStr);
    }

    // if (!SUCCEEDED(hr)){
    //     delete this;
    // }
}


void WindowsToastNotification::NotificationClicked()
{
    // Local<Function> fn = Local<Function>::New(Isolate::GetCurrent(), m_cbFn);
    // Handle<Value> args[1];
    // fn->Call(Isolate::GetCurrent()->GetCurrentContext()->Global(), 0, args);
    // delete this;
}


void WindowsToastNotification::NotificationDismissed()
{
    delete this;
}


HRESULT WindowsToastNotification::GetToastXml(IToastNotificationManagerStatics* toastMgr, const WCHAR* title, const WCHAR* msg, const char* img, IXmlDocument** toastXml)
{
    HRESULT hr;
    ToastTemplateType templateType;
    if (title == NULL || msg == NULL){
        // Single line toast
        templateType = img == NULL ? ToastTemplateType_ToastText01 : ToastTemplateType_ToastImageAndText01;
        hr = toastMgr->GetTemplateContent(templateType, toastXml);
        if (SUCCEEDED(hr)) {
            const WCHAR* text = title != NULL ? title : msg;
            hr = SetXmlText(*toastXml, text);
        }
    } else {
        // Title and body toast
        templateType = img == NULL ? ToastTemplateType_ToastText02 : ToastTemplateType_ToastImageAndText02;
        hr = toastMgr->GetTemplateContent(templateType, toastXml);
        if (SUCCEEDED(hr)) {
            hr = SetXmlText(*toastXml, title, msg);
        }
    }

    if (img != NULL && SUCCEEDED(hr)){
        // Toast has image
        hr = CreateTempImageFile(img);
        if (SUCCEEDED(hr)){
            hr = SetXmlImage(*toastXml);
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
    do{
        BREAK_IF_BAD(hr);

        ComPtr<IXmlNode> node;
        hr = nodeList->Item(0, &node);
        BREAK_IF_BAD(hr);

        hr = AppendTextToXml(doc, node.Get(), text);
    } while (FALSE);

    if (tag != NULL){
        WindowsDeleteString(tag);
    }

    return hr;
}


HRESULT WindowsToastNotification::SetXmlText(IXmlDocument* doc, const WCHAR* title, const WCHAR* body)
{
    HSTRING tag = NULL;
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = GetTextNodeList(&tag, doc, &nodeList, 2);
    do{
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

    if (tag != NULL){
        WindowsDeleteString(tag);
    }

    return hr;
}


HRESULT WindowsToastNotification::SetXmlImage(IXmlDocument* doc)
{
    HSTRING tag = NULL;
    HSTRING src = NULL;
    HSTRING imgPath = NULL;
    HRESULT hr = CreateHString(L"image", &tag);
    do{
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
        swprintf(xmlPath, ARRAYSIZE(xmlPath), L"file:///%S", m_tempFilePath);
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

    if (tag != NULL){
        WindowsDeleteString(tag);
    }
    if (src != NULL){
        WindowsDeleteString(src);
    }
    if (imgPath != NULL){
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

    if (!SUCCEEDED(hr)){
        // Allow the caller to delete this string on success
        WindowsDeleteString(*tag);
    }

    return hr;
}


HRESULT WindowsToastNotification::AppendTextToXml(IXmlDocument* doc, IXmlNode* node, const WCHAR* text)
{
    HSTRING str = NULL;
    HRESULT hr = CreateHString(text, &str);
    do{
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

    if (str != NULL){
        WindowsDeleteString(str);
    }

    return hr;
}


HRESULT WindowsToastNotification::SetupCallbacks(IToastNotification* toast)
{
    EventRegistrationToken activatedToken, dismissedToken;
    m_eventHandler = new ToastEventHandler(this);
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


HRESULT WindowsToastNotification::CreateTempImageFile(const char* base64)
{
    BYTE* buff = NULL;
    HRESULT hr = EnsureTempDir();
    do{
        BREAK_IF_BAD(hr);

        DWORD buffLen;
        hr = ConvertBase64DataToImage(base64, &buff, &buffLen);
        BREAK_IF_BAD(hr);

        hr = WriteToFile(buff, buffLen);
    } while (FALSE);

    if (buff != NULL){
        delete buff;
    }

    return hr;
}


HRESULT WindowsToastNotification::ConvertBase64DataToImage(const char* base64, BYTE** buff, DWORD* buffLen)
{
    HRESULT hr = E_FAIL;
    *buffLen = 0;
    DWORD reqSize;
    if (CryptStringToBinary((LPCWSTR)base64, 0, CRYPT_STRING_BASE64, NULL, &reqSize, NULL, NULL)){
        *buff = new BYTE[reqSize];
        if (CryptStringToBinary((LPCWSTR)base64, 0, CRYPT_STRING_BASE64, *buff, &reqSize, NULL, NULL)){
            *buffLen = reqSize;
            hr = S_OK;
        }
    }

    return hr;
}


HRESULT WindowsToastNotification::WriteToFile(BYTE* buff, DWORD buffLen)
{
    HRESULT hr = E_FAIL;

    GUID guid;
    hr = CoCreateGuid(&guid);
    if (SUCCEEDED(hr)){
        WCHAR randomName[MAX_PATH];
        StringFromGUID2(guid, randomName, ARRAYSIZE(randomName));

        m_tempFilePath = new char[MAX_PATH];
        sprintf(m_tempFilePath, "%s\\%S", s_tempDirPath, randomName);
        HANDLE file = CreateFile((LPCWSTR)m_tempFilePath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file != INVALID_HANDLE_VALUE){
            DWORD bytesWritten;
            if (WriteFile(file, buff, buffLen, &bytesWritten, NULL)){
                hr = S_OK;
            }
            else{
                DeleteTempImageFile();
            }

            CloseHandle(file);
        }
    }

    if (!SUCCEEDED(hr) && m_tempFilePath){
        delete m_tempFilePath;
        m_tempFilePath = NULL;
    }


    return hr;
}


HRESULT WindowsToastNotification::EnsureTempDir()
{
    // Check for the Dir already existing or easy creation
    HRESULT hr = E_FAIL;
    if (CreateDirectory((LPCWSTR)s_tempDirPath, NULL) || GetLastError() == ERROR_ALREADY_EXISTS){
        hr = S_OK;
    } else {
        // NOTE: This makes an assumption that the path is a FULL LENGTH PATH.  Therefore the first folder starts at the
        // 4th character.  ie. "c:\path\to\temp\dir"

        // It's possible that multiple directories need to be created.
        for (int i = 3; i < ARRAYSIZE(s_tempDirPath); i++) {
            char c = s_tempDirPath[i];
            if (c == '\\'){
                //Substring until this char
                char* temp = new char[i + 1];
                strncpy(temp, s_tempDirPath, i);
                temp[i] = 0;
                if (!CreateDirectory((LPCWSTR)temp, NULL) && GetLastError() != ERROR_ALREADY_EXISTS){
                    delete temp;
                    return hr;
                }

                delete temp;
            }
        }

        //Try to create the full path one more time.  This will take care of paths that don't end with a slash
        if (CreateDirectory((LPCWSTR)s_tempDirPath, NULL) || GetLastError() == ERROR_ALREADY_EXISTS){
            hr = S_OK;
        }
    }

    return hr;
}


HRESULT WindowsToastNotification::DeleteTempImageFile()
{
    if (DeleteFile((LPCWSTR)m_tempFilePath)){
        return S_OK;
    }

    return E_FAIL;
}


HRESULT WindowsToastNotification::EnsureShortcut()
{   
    HRESULT hr = E_FAIL;
    char shortcut[MAX_PATH];
    DWORD charsWritten = GetEnvironmentVariable(L"APPDATA", (LPWSTR)shortcut, MAX_PATH);

    if (charsWritten > 0) {
        char shortcutCat[MAX_PATH];
        sprintf(shortcutCat, SHORTCUT_FORMAT, s_appName);
        errno_t concatErr = strcat_s(shortcut, ARRAYSIZE(shortcut), shortcutCat);
        if (concatErr == 0) {
            DWORD attr = GetFileAttributes((LPCWSTR)shortcut);
            bool exists = attr < 0xFFFFFFF;
            if (exists){
                hr = S_OK;
            }
            else{
                WCHAR path[MAX_PATH];
                mbstowcs(path, shortcut, ARRAYSIZE(path));
                hr = CreateShortcut(path);
            }
        }
    }

    return hr;
}


HRESULT WindowsToastNotification::CreateShortcut(WCHAR* path)
{
    HRESULT hr = E_FAIL;
    char exePath[MAX_PATH];
    DWORD charsWritten = GetModuleFileNameEx(GetCurrentProcess(), nullptr, (LPWSTR)exePath, ARRAYSIZE(exePath));
    if (charsWritten > 0) {
        PROPVARIANT propVariant;
        ComPtr<IShellLink> shellLink;
        hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
        do{
            BREAK_IF_BAD(hr);

            hr = shellLink->SetPath((LPCWSTR)exePath);
            BREAK_IF_BAD(hr);

            ComPtr<IPropertyStore> propStore;
            hr = shellLink.As(&propStore);
            BREAK_IF_BAD(hr);

            WCHAR wAppName[MAX_PATH];
            swprintf(wAppName, ARRAYSIZE(wAppName), L"%S", s_appName);
            hr = InitPropVariantFromString(wAppName, &propVariant);
            BREAK_IF_BAD(hr);

            hr = propStore->SetValue(PKEY_AppUserModel_ID, propVariant);
            BREAK_IF_BAD(hr);

            hr = propStore->Commit();
            BREAK_IF_BAD(hr);

            ComPtr<IPersistFile> persistFile;
            hr = shellLink.As(&persistFile);
            BREAK_IF_BAD(hr);

            hr = persistFile->Save(path, TRUE);
        } while (FALSE);

        PropVariantClear(&propVariant);
    }

    return hr;
}

ToastEventHandler::ToastEventHandler(WindowsToastNotification* notification)
{
    m_ref = 1;
    m_notification = notification;
}


ToastEventHandler::~ToastEventHandler()
{
    // Empty
}


IFACEMETHODIMP ToastEventHandler::Invoke(IToastNotification* sender, IInspectable* args)
{
    //Notification "activated" (clicked)
    if (m_notification != NULL){
        m_notification->NotificationClicked();
    }
    return S_OK;
}


IFACEMETHODIMP ToastEventHandler::Invoke(IToastNotification* sender, IToastDismissedEventArgs* e)
{
    //Notification dismissed
    if (m_notification != NULL){
        m_notification->NotificationDismissed();
    }
    return S_OK;
}

} //namespace