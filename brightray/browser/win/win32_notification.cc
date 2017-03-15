#define WIN32_LEAN_AND_MEAN
#include "win32_notification.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include <windows.h>

namespace brightray {

void Win32Notification::Show(const base::string16& title, const base::string16& msg, const std::string& tag, const GURL& icon_url, const SkBitmap& icon, const bool silent)
{
    auto presenter = static_cast<NotificationPresenterWin7*>(this->presenter());
    if(!presenter) return;

    HBITMAP image = NULL;

    if(!icon.drawsNothing())
    {
        if(icon.colorType() == kBGRA_8888_SkColorType)
        {
            icon.lockPixels();

            BITMAPINFOHEADER bmi = { sizeof(BITMAPINFOHEADER) };
            bmi.biWidth = icon.width();
            bmi.biHeight = -icon.height();
            bmi.biPlanes = 1;
            bmi.biBitCount = 32;
            bmi.biCompression = BI_RGB;

            HDC hdcScreen = GetDC(NULL);
            image = CreateDIBitmap(hdcScreen, &bmi, CBM_INIT, icon.getPixels(), (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
            ReleaseDC(NULL, hdcScreen);

            icon.unlockPixels();
        }
    }

    Win32Notification* existing = nullptr;
    if(!tag.empty()) existing = presenter->GetNotificationObjectByTag(tag);

    if(existing)
    {
        existing->tag.clear();
        this->notificationRef = std::move(existing->notificationRef);
        this->notificationRef.Set(title, msg, image);
    }
    else
    {
        this->notificationRef = presenter->AddNotification(title, msg, image);
    }

    this->tag = tag;

    if(image) DeleteObject(image);
}

void Win32Notification::Dismiss()
{
    notificationRef.Close();
}

}
