#pragma once
#include <Windows.h>

namespace brightray {

struct NotificationData
{
    DesktopNotificationController* controller = nullptr;

    std::wstring caption;
    std::wstring bodyText;
    HBITMAP image = NULL;


    NotificationData() = default;

    ~NotificationData()
    {
        if(image) DeleteObject(image);
    }

    NotificationData(const NotificationData& other) = delete;
    NotificationData& operator=(const NotificationData& other) = delete;
};

template<typename T>
inline T ScaleForDpi(T value, unsigned dpi)
{
    return value * dpi / 96;
}

struct ScreenMetrics
{
    UINT dpiX, dpiY;

    ScreenMetrics()
    {
        typedef HRESULT WINAPI GetDpiForMonitor_t(HMONITOR, int, UINT*, UINT*);
        auto GetDpiForMonitor = (GetDpiForMonitor_t*)GetProcAddress(GetModuleHandle(TEXT("shcore")), "GetDpiForMonitor");
        if(GetDpiForMonitor)
        {
            auto monitor = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);
            if(GetDpiForMonitor(monitor, 0, &dpiX, &dpiY) == S_OK)
                return;
        }

        HDC hdc = GetDC(NULL);
        dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
    }

    template<typename T> T X(T value) const { return ScaleForDpi(value, dpiX); }
    template<typename T> T Y(T value) const { return ScaleForDpi(value, dpiY); }
};

}
