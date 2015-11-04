#ifndef __WINDOWS_TOAST_NOTIFICATION_H__
#define __WINDOWS_TOAST_NOTIFICATION_H__

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windows.ui.notifications.h>
#include <wrl/client.h>
#include <wrl/implements.h>

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
        static char s_tempDirPath[MAX_PATH];

        char* m_tempFilePath;
        ToastEventHandler* m_eventHandler;

        HRESULT GetToastXml(IToastNotificationManagerStatics* toastMgr, const WCHAR* title, const WCHAR* msg, const char* img, IXmlDocument** toastXml);
        HRESULT SetXmlText(IXmlDocument* doc, const WCHAR* text);
        HRESULT SetXmlText(IXmlDocument* doc, const WCHAR* title, const WCHAR* body);
        HRESULT SetXmlImage(IXmlDocument* doc);
        HRESULT GetTextNodeList(HSTRING* tag, IXmlDocument* doc, IXmlNodeList** nodeList, UINT32 reqLength);
        HRESULT AppendTextToXml(IXmlDocument* doc, IXmlNode* node, const WCHAR* text);
        HRESULT SetupCallbacks(IToastNotification* toast);
        HRESULT CreateHString(const WCHAR* source, HSTRING* dest);
        HRESULT CreateTempImageFile(const char* base64);
        HRESULT ConvertBase64DataToImage(const char* base64, BYTE** buff, DWORD* buffLen);
        HRESULT WriteToFile(BYTE* buff, DWORD buffLen);
        HRESULT EnsureTempDir();
        HRESULT DeleteTempImageFile();
        HRESULT EnsureShortcut();
        HRESULT CreateShortcut(WCHAR* path);

    public:
        WindowsToastNotification(const char* appName);
        ~WindowsToastNotification();
        void ShowNotification(const WCHAR* title, const WCHAR* msg, const char* img);
        void NotificationClicked();
        void NotificationDismissed();

        static void InitSystemProps(char* appName, char* tempDirPath);
    };


    class ToastEventHandler :
        public Implements < DesktopToastActivatedEventHandler, DesktopToastDismissedEventHandler >
    {
    private:
        ULONG m_ref;
        WindowsToastNotification* m_notification;

    public:
        ToastEventHandler(WindowsToastNotification* notification);
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