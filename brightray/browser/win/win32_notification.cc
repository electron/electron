#define WIN32_LEAN_AND_MEAN

#include "brightray/browser/win/win32_notification.h"

#include <windows.h>
#include <string>

#include "third_party/skia/include/core/SkBitmap.h"

namespace brightray {

void Win32Notification::Show(
    const base::string16& title, const base::string16& msg,
    const std::string& tag, const GURL& icon_url,
    const SkBitmap& icon, bool silent,
    bool has_reply, const base::string16& reply_placeholder) {
    auto presenter = static_cast<NotificationPresenterWin7*>(this->presenter());
    if (!presenter) return;

    HBITMAP image = NULL;

    if (!icon.drawsNothing()) {
        if (icon.colorType() == kBGRA_8888_SkColorType) {
            icon.lockPixels();

            BITMAPINFOHEADER bmi = { sizeof(BITMAPINFOHEADER) };
            bmi.biWidth = icon.width();
            bmi.biHeight = -icon.height();
            bmi.biPlanes = 1;
            bmi.biBitCount = 32;
            bmi.biCompression = BI_RGB;

            HDC hdcScreen = GetDC(NULL);
            image = CreateDIBitmap(hdcScreen, &bmi, CBM_INIT, icon.getPixels(),
                                   reinterpret_cast<BITMAPINFO*>(&bmi),
                                   DIB_RGB_COLORS);
            ReleaseDC(NULL, hdcScreen);

            icon.unlockPixels();
        }
    }

    Win32Notification* existing = nullptr;
    if (!tag.empty()) existing = presenter->GetNotificationObjectByTag(tag);

    if (existing) {
        existing->tag_.clear();
        this->notification_ref_ = std::move(existing->notification_ref_);
        this->notification_ref_.Set(title, msg, image);
    } else {
        this->notification_ref_ = presenter->AddNotification(title, msg, image);
    }

    this->tag_ = tag;

    if (image) DeleteObject(image);
}

void Win32Notification::Dismiss() {
    notification_ref_.Close();
}

}   // namespace brightray
