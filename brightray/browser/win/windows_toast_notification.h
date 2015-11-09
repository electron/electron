// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon <jason.poon@microsoft.com>. All rights reserved.
// Copyright (c) 2015 Ryan McShane <rmcshane@bandwidth.com> and Brandon Smith <bsmith@bandwidth.com>
// Thanks to both of those folks mentioned above who first thought up a bunch of this code
// and released it as MIT to the world.

#ifndef __WINDOWS_TOAST_NOTIFICATION_H__
#define __WINDOWS_TOAST_NOTIFICATION_H__

#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/platform_notification_data.h"
#include "base/bind.h"

#include <windows.h>
#include <windows.ui.notifications.h>
#include <wrl/implements.h>

using namespace Microsoft::WRL;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Foundation;

namespace WinToasts {

    typedef ITypedEventHandler<ToastNotification*, IInspectable*> DesktopToastActivatedEventHandler;
    typedef ITypedEventHandler<ToastNotification*, ToastDismissedEventArgs*> DesktopToastDismissedEventHandler;

    class ToastEventHandler;

    class WindowsToastNotification
    {
    public:
        WindowsToastNotification(const char* appName, content::DesktopNotificationDelegate* delegate);
        ~WindowsToastNotification();
        void ShowNotification(const WCHAR* title, const WCHAR* msg, std::string iconPath, ComPtr<IToastNotification>& toast);
        void DismissNotification(ComPtr<IToastNotification> toast);
        void NotificationClicked();
        void NotificationDismissed();

    private:
        ToastEventHandler* m_eventHandler;

        content::DesktopNotificationDelegate* n_delegate;
        ComPtr<IToastNotificationManagerStatics> m_toastManager;
        ComPtr<IToastNotifier> m_toastNotifier;

        HRESULT GetToastXml(IToastNotificationManagerStatics* toastManager, const WCHAR* title, const WCHAR* msg, std::string iconPath, ABI::Windows::Data::Xml::Dom::IXmlDocument** toastXml);
        HRESULT SetXmlText(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc, const WCHAR* text);
        HRESULT SetXmlText(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc, const WCHAR* title, const WCHAR* body);
        HRESULT SetXmlImage(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc, std::string iconPath);
        HRESULT GetTextNodeList(HSTRING* tag, ABI::Windows::Data::Xml::Dom::IXmlDocument* doc, ABI::Windows::Data::Xml::Dom::IXmlNodeList** nodeList, UINT32 reqLength);
        HRESULT AppendTextToXml(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc, ABI::Windows::Data::Xml::Dom::IXmlNode* node, const WCHAR* text);
        HRESULT SetupCallbacks(IToastNotification* toast);
        HRESULT CreateHString(const WCHAR* source, HSTRING* dest);
    };


    class ToastEventHandler :
        public Implements <DesktopToastActivatedEventHandler, DesktopToastDismissedEventHandler>
    {
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

    private:
        ULONG m_ref;
        WindowsToastNotification* m_notification;
        content::DesktopNotificationDelegate* n_delegate;
    };

} // namespace

#endif //__WINDOWS_TOAST_NOTIFICATION_H__