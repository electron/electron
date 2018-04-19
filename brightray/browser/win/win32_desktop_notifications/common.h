#pragma once
#include <Windows.h>

namespace brightray {

struct NotificationData {
  DesktopNotificationController* controller = nullptr;

  std::wstring caption;
  std::wstring body_text;
  HBITMAP image = NULL;

  NotificationData() = default;

  ~NotificationData() {
    if (image)
      DeleteObject(image);
  }

  NotificationData(const NotificationData& other) = delete;
  NotificationData& operator=(const NotificationData& other) = delete;
};

template <typename T>
constexpr T ScaleForDpi(T value, unsigned dpi, unsigned source_dpi = 96) {
  return value * dpi / source_dpi;
}

struct ScreenMetrics {
  UINT dpi_x, dpi_y;

  ScreenMetrics() {
    typedef HRESULT WINAPI GetDpiForMonitor_t(HMONITOR, int, UINT*, UINT*);

    auto GetDpiForMonitor = reinterpret_cast<GetDpiForMonitor_t*>(
        GetProcAddress(GetModuleHandle(TEXT("shcore")), "GetDpiForMonitor"));

    if (GetDpiForMonitor) {
      auto monitor = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);
      if (GetDpiForMonitor(monitor, 0, &dpi_x, &dpi_y) == S_OK)
        return;
    }

    HDC hdc = GetDC(NULL);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
  }

  template <class T>
  T X(T value) const {
    return ScaleForDpi(value, dpi_x);
  }
  template <class T>
  T Y(T value) const {
    return ScaleForDpi(value, dpi_y);
  }
};

}  // namespace brightray
