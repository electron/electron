// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com>. All rights reserved.
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of this code
// and released it as MIT to the world.

#ifndef __WINDOWS_TOAST_NOTIFICATION_H__
#define __WINDOWS_TOAST_NOTIFICATION_H__

#define WIN32_LEAN_AND_MEAN

#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/platform_notification_data.h"
#include "base/bind.h"

#include <windows.h>
#include <windows.ui.notifications.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <string>

using namespace Microsoft::WRL;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Data::Xml::Dom;

namespace WinToasts{

    typedef ITypedEventHandler<ToastNotification*, IInspectable*> DesktopToastActivatedEventHandler;
    typedef ITypedEventHandler<ToastNotification*, ToastDismissedEventArgs*> DesktopToastDismissedEventHandler;

    class ToastEventHandler;

    class WindowsToastNotification
    {
    private:
        static char s_appName[MAX_PATH];
        ToastEventHandler* m_eventHandler;
        content::DesktopNotificationDelegate* n_delegate;

        HRESULT GetToastXml(IToastNotificationManagerStatics* toastMgr, const WCHAR* title, const WCHAR* msg, std::string iconPath, IXmlDocument** toastXml);
        HRESULT SetXmlText(IXmlDocument* doc, const WCHAR* text);
        HRESULT SetXmlText(IXmlDocument* doc, const WCHAR* title, const WCHAR* body);
        HRESULT SetXmlImage(IXmlDocument* doc, std::string iconPath);
        HRESULT GetTextNodeList(HSTRING* tag, IXmlDocument* doc, IXmlNodeList** nodeList, UINT32 reqLength);
        HRESULT AppendTextToXml(IXmlDocument* doc, IXmlNode* node, const WCHAR* text);
        HRESULT SetupCallbacks(IToastNotification* toast);
        HRESULT CreateHString(const WCHAR* source, HSTRING* dest);

    public:
        WindowsToastNotification(const char* appName, content::DesktopNotificationDelegate* delegate);
        ~WindowsToastNotification();
        void ShowNotification(const WCHAR* title, const WCHAR* msg, std::string iconPath);
        void NotificationClicked();
        void NotificationDismissed();
    };


    class ToastEventHandler :
        public Implements < DesktopToastActivatedEventHandler, DesktopToastDismissedEventHandler >
    {
    private:
        ULONG m_ref;
        WindowsToastNotification* m_notification;
        content::DesktopNotificationDelegate* n_delegate;

    public:
        ToastEventHandler(WindowsToastNotification* notification, content::DesktopNotificationDelegate* delegate);
        ~ToastEventHandler();
        IFACEMETHODIMP Invoke(IToastNotification* sender, IInspectable* args);
        IFACEMETHODIMP Invoke(IToastNotification* sender, IToastDismissedEventArgs* e);
        IFACEMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_ref); }

        IFACEMETHODIMP_(ULONG) Release() {
            ULONG l = InterlockedDecrement(&m_ref);
            if (l == 0) delete this;
            return l;
        }

        IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_ void **ppv) {
            if (IsEqualIID(riid, IID_IUnknown))
                *ppv = static_cast<IUnknown*>(static_cast<DesktopToastActivatedEventHandler*>(this));
            else if (IsEqualIID(riid, __uuidof(DesktopToastActivatedEventHandler)))
                *ppv = static_cast<DesktopToastActivatedEventHandler*>(this);
            else if (IsEqualIID(riid, __uuidof(DesktopToastDismissedEventHandler)))
                *ppv = static_cast<DesktopToastDismissedEventHandler*>(this);
            else *ppv = nullptr;

            if (*ppv) {
                reinterpret_cast<IUnknown*>(*ppv)->AddRef();
                return S_OK;
            }

            return E_NOINTERFACE;
        }
    };

} // namespace

#endif //__WINDOWS_TOAST_NOTIFICATION_H__