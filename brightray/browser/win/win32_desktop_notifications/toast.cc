#define NOMINMAX
#include "browser/win/win32_desktop_notifications/toast.h"
#include <uxtheme.h>
#include <windowsx.h>
#include <algorithm>
#include "browser/win/win32_desktop_notifications/common.h"

#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "uxtheme.lib")

using std::min;
using std::shared_ptr;

namespace brightray {

static COLORREF GetAccentColor() {
    bool success = false;
    if (IsAppThemed()) {
        HKEY hkey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER,
                         TEXT("SOFTWARE\\Microsoft\\Windows\\DWM"), 0,
                         KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {
            COLORREF color;
            DWORD type, size;
            if (RegQueryValueEx(hkey, TEXT("AccentColor"), nullptr,
                                &type,
                                reinterpret_cast<BYTE*>(&color),
                                &(size = sizeof(color))) == ERROR_SUCCESS &&
                type == REG_DWORD) {
                // convert from RGBA
                color = RGB(GetRValue(color),
                            GetGValue(color),
                            GetBValue(color));
                success = true;
            }
            else if (
                RegQueryValueEx(hkey, TEXT("ColorizationColor"), nullptr,
                                &type,
                                reinterpret_cast<BYTE*>(&color),
                                &(size = sizeof(color))) == ERROR_SUCCESS &&
                type == REG_DWORD) {
                // convert from BGRA
                color = RGB(GetBValue(color),
                            GetGValue(color),
                            GetRValue(color));
                success = true;
            }

            RegCloseKey(hkey);

            if (success) return color;
        }
    }

    return GetSysColor(COLOR_ACTIVECAPTION);
}

// Stretches a bitmap to the specified size, preserves alpha channel
static HBITMAP StretchBitmap(HBITMAP bitmap, unsigned width, unsigned height) {
    // We use StretchBlt for the scaling, but that discards the alpha channel.
    // So we first create a separate grayscale bitmap from the alpha channel,
    // scale that separately, and copy it back to the scaled color bitmap.

    BITMAP bm;
    if (!GetObject(bitmap, sizeof(bm), &bm))
        return NULL;

    if (width == 0 || height == 0)
        return NULL;

    HBITMAP resultBitmap = NULL;

    HDC hdcScreen = GetDC(NULL);

    HBITMAP alphaSrcBitmap;
    {
        BITMAPINFOHEADER bmi = { sizeof(BITMAPINFOHEADER) };
        bmi.biWidth = bm.bmWidth;
        bmi.biHeight = bm.bmHeight;
        bmi.biPlanes = bm.bmPlanes;
        bmi.biBitCount = bm.bmBitsPixel;
        bmi.biCompression = BI_RGB;

        void* alphaSrcBits;
        alphaSrcBitmap = CreateDIBSection(NULL,
                                          reinterpret_cast<BITMAPINFO*>(&bmi),
                                          DIB_RGB_COLORS, &alphaSrcBits,
                                          NULL, 0);

        if (alphaSrcBitmap) {
            if (GetDIBits(hdcScreen, bitmap, 0, 0, 0,
                          reinterpret_cast<BITMAPINFO*>(&bmi),
                          DIB_RGB_COLORS) &&
                bmi.biSizeImage > 0 &&
                (bmi.biSizeImage % 4) == 0) {
                auto buf = reinterpret_cast<BYTE*>(
                    _aligned_malloc(bmi.biSizeImage, sizeof(DWORD)));

                if (buf) {
                    GetDIBits(hdcScreen, bitmap, 0, bm.bmHeight, buf,
                              reinterpret_cast<BITMAPINFO*>(&bmi),
                              DIB_RGB_COLORS);

                    const DWORD *src = reinterpret_cast<DWORD*>(buf);
                    const DWORD *end =
                        reinterpret_cast<DWORD*>(buf + bmi.biSizeImage);

                    BYTE* dest = reinterpret_cast<BYTE*>(alphaSrcBits);

                    for (; src != end; ++src, ++dest) {
                        BYTE a = *src >> 24;
                        *dest++ = a;
                        *dest++ = a;
                        *dest++ = a;
                    }

                    _aligned_free(buf);
                }
            }
        }
    }

    if (alphaSrcBitmap) {
        BITMAPINFOHEADER bmi = { sizeof(BITMAPINFOHEADER) };
        bmi.biWidth = width;
        bmi.biHeight = height;
        bmi.biPlanes = 1;
        bmi.biBitCount = 32;
        bmi.biCompression = BI_RGB;

        void* colorBits;
        auto colorBitmap = CreateDIBSection(NULL,
                                            reinterpret_cast<BITMAPINFO*>(&bmi),
                                            DIB_RGB_COLORS, &colorBits,
                                            NULL, 0);

        void* alphaBits;
        auto alphaBitmap = CreateDIBSection(NULL,
                                            reinterpret_cast<BITMAPINFO*>(&bmi),
                                            DIB_RGB_COLORS, &alphaBits,
                                            NULL, 0);

        HDC hdc = CreateCompatibleDC(NULL);
        HDC hdcSrc = CreateCompatibleDC(NULL);

        if (colorBitmap && alphaBitmap && hdc && hdcSrc) {
            SetStretchBltMode(hdc, HALFTONE);

            // resize color channels
            SelectObject(hdc, colorBitmap);
            SelectObject(hdcSrc, bitmap);
            StretchBlt(hdc, 0, 0, width, height,
                       hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight,
                       SRCCOPY);

            // resize alpha channel
            SelectObject(hdc, alphaBitmap);
            SelectObject(hdcSrc, alphaSrcBitmap);
            StretchBlt(hdc, 0, 0, width, height,
                       hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight,
                       SRCCOPY);

            // flush before touching the bits
            GdiFlush();

            // apply the alpha channel
            auto dest = reinterpret_cast<BYTE*>(colorBits);
            auto src = reinterpret_cast<const BYTE*>(alphaBits);
            auto end = src + (width * height * 4);
            while (src != end) {
                dest[3] = src[0];
                dest += 4;
                src += 4;
            }

            // create the resulting bitmap
            resultBitmap = CreateDIBitmap(hdcScreen, &bmi, CBM_INIT,
                                          colorBits,
                                          reinterpret_cast<BITMAPINFO*>(&bmi),
                                          DIB_RGB_COLORS);
        }

        if (hdcSrc) DeleteDC(hdcSrc);
        if (hdc) DeleteDC(hdc);

        if (alphaBitmap) DeleteObject(alphaBitmap);
        if (colorBitmap) DeleteObject(colorBitmap);

        DeleteObject(alphaSrcBitmap);
    }

    ReleaseDC(NULL, hdcScreen);

    return resultBitmap;
}

DesktopNotificationController::Toast::Toast(
    HWND hWnd, shared_ptr<NotificationData>* data) :
    hwnd_(hWnd), data_(*data) {
    HDC hdcScreen = GetDC(NULL);
    hdc_ = CreateCompatibleDC(hdcScreen);
    ReleaseDC(NULL, hdcScreen);
}

DesktopNotificationController::Toast::~Toast() {
    DeleteDC(hdc_);
    if (bitmap_) DeleteBitmap(bitmap_);
    if (scaled_image_) DeleteBitmap(scaled_image_);
}

void DesktopNotificationController::Toast::Register(HINSTANCE hInstance) {
    WNDCLASSEX wc = { sizeof(wc) };
    wc.lpfnWndProc = &Toast::WndProc;
    wc.lpszClassName = class_name_;
    wc.cbWndExtra = sizeof(Toast*);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wc);
}

LRESULT DesktopNotificationController::Toast::WndProc(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        {
            auto& cs = reinterpret_cast<const CREATESTRUCT*&>(lParam);
            auto data =
                static_cast<shared_ptr<NotificationData>*>(cs->lpCreateParams);
            auto inst = new Toast(hWnd, data);
            SetWindowLongPtr(hWnd, 0, (LONG_PTR)inst);
        }
        break;

    case WM_NCDESTROY:
        delete Get(hWnd);
        SetWindowLongPtr(hWnd, 0, 0);
        return 0;

    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;

    case WM_TIMER:
        if (wParam == TimerID_AutoDismiss) {
            Get(hWnd)->AutoDismiss();
        }
        return 0;

    case WM_LBUTTONDOWN:
        {
            auto inst = Get(hWnd);

            inst->Dismiss();

            Notification notification(inst->data_);
            if (inst->is_close_hot_)
                inst->data_->controller->OnNotificationDismissed(notification);
            else
                inst->data_->controller->OnNotificationClicked(notification);
        }
        return 0;

    case WM_MOUSEMOVE:
        {
            auto inst = Get(hWnd);
            if (!inst->is_highlighted_) {
                inst->is_highlighted_ = true;

                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd };
                TrackMouseEvent(&tme);
            }

            POINT cursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            inst->is_close_hot_ =
                (PtInRect(&inst->close_button_rect_, cursor) != FALSE);

            if (!inst->is_non_interactive_)
                inst->CancelDismiss();

            inst->UpdateContents();
        }
        return 0;

    case WM_MOUSELEAVE:
        {
            auto inst = Get(hWnd);
            inst->is_highlighted_ = false;
            inst->is_close_hot_ = false;
            inst->UpdateContents();

            if (!inst->ease_out_active_ && inst->ease_in_pos_ == 1.0f)
                inst->ScheduleDismissal();

            // Make sure stack collapse happens if needed
            inst->data_->controller->StartAnimation();
        }
        return 0;

    case WM_WINDOWPOSCHANGED:
        {
            auto& wp = reinterpret_cast<WINDOWPOS*&>(lParam);
            if (wp->flags & SWP_HIDEWINDOW) {
                if (!IsWindowVisible(hWnd))
                    Get(hWnd)->is_highlighted_ = false;
            }
        }
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND DesktopNotificationController::Toast::Create(
    HINSTANCE hInstance, shared_ptr<NotificationData>& data) {
    return CreateWindowEx(WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
                          class_name_, nullptr, WS_POPUP, 0, 0, 0, 0,
                          NULL, NULL, hInstance, &data);
}

void DesktopNotificationController::Toast::Draw() {
    const COLORREF accent = GetAccentColor();

    COLORREF backColor;
    {
        // base background color is 2/3 of accent
        // highlighted adds a bit of intensity to every channel

        int h = is_highlighted_ ? (0xff / 20) : 0;

        backColor = RGB(min(0xff, (GetRValue(accent) * 2 / 3) + h),
                        min(0xff, (GetGValue(accent) * 2 / 3) + h),
                        min(0xff, (GetBValue(accent) * 2 / 3) + h));
    }

    const float backLuma =
        (GetRValue(backColor) * 0.299f / 255) +
        (GetGValue(backColor) * 0.587f / 255) +
        (GetBValue(backColor) * 0.114f / 255);

    const struct { float r, g, b; } backF = {
        GetRValue(backColor) / 255.0f,
        GetGValue(backColor) / 255.0f,
        GetBValue(backColor) / 255.0f,
    };

    COLORREF foreColor, dimmedColor;
    {
        // based on the lightness of background, we draw foreground in light
        // or dark shades of gray blended onto the background with slight
        // transparency to avoid sharp contrast

        constexpr float alpha = 0.9f;
        constexpr float intensityLight[] = { (1.0f * alpha), (0.8f * alpha) };
        constexpr float intensityDark[] = { (0.1f * alpha), (0.3f * alpha) };

        // select foreground intensity values (light or dark)
        auto& i = (backLuma < 0.6f) ? intensityLight : intensityDark;

        float r, g, b;

        r = i[0] + backF.r * (1 - alpha);
        g = i[0] + backF.g * (1 - alpha);
        b = i[0] + backF.b * (1 - alpha);
        foreColor = RGB(r * 0xff, g * 0xff, b * 0xff);

        r = i[1] + backF.r * (1 - alpha);
        g = i[1] + backF.g * (1 - alpha);
        b = i[1] + backF.b * (1 - alpha);
        dimmedColor = RGB(r * 0xff, g * 0xff, b * 0xff);
    }

    // Draw background
    {
        auto brush = CreateSolidBrush(backColor);

        RECT rc = { 0, 0, toast_size_.cx, toast_size_.cy };
        FillRect(hdc_, &rc, brush);

        DeleteBrush(brush);
    }

    SetBkMode(hdc_, TRANSPARENT);

    const auto close = L'\x2715';
    auto captionFont = data_->controller->GetCaptionFont();
    auto bodyFont = data_->controller->GetBodyFont();

    TEXTMETRIC tmCap;
    SelectFont(hdc_, captionFont);
    GetTextMetrics(hdc_, &tmCap);

    auto textOffsetX = margin_.cx;

    BITMAP imageInfo = {};
    if (scaled_image_) {
        GetObject(scaled_image_, sizeof(imageInfo), &imageInfo);

        textOffsetX += margin_.cx + imageInfo.bmWidth;
    }

    // calculate close button rect
    POINT closePos;
    {
        SIZE extent = {};
        GetTextExtentPoint32W(hdc_, &close, 1, &extent);

        close_button_rect_.right = toast_size_.cx;
        close_button_rect_.top = 0;

        closePos.x = close_button_rect_.right - margin_.cy - extent.cx;
        closePos.y = close_button_rect_.top + margin_.cy;

        close_button_rect_.left = closePos.x - margin_.cy;
        close_button_rect_.bottom = closePos.y + extent.cy + margin_.cy;
    }

    // image
    if (scaled_image_) {
        HDC hdcImage = CreateCompatibleDC(NULL);
        SelectBitmap(hdcImage, scaled_image_);
        BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
        AlphaBlend(hdc_, margin_.cx, margin_.cy,
                   imageInfo.bmWidth, imageInfo.bmHeight,
                   hdcImage, 0, 0,
                   imageInfo.bmWidth, imageInfo.bmHeight,
                   blend);
        DeleteDC(hdcImage);
    }

    // caption
    {
        RECT rc = {
            textOffsetX,
            margin_.cy,
            close_button_rect_.left,
            toast_size_.cy
        };

        SelectFont(hdc_, captionFont);
        SetTextColor(hdc_, foreColor);
        DrawText(hdc_, data_->caption.data(), (UINT)data_->caption.length(),
                 &rc, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    // body text
    if (!data_->body_text.empty()) {
        RECT rc = {
            textOffsetX,
            2 * margin_.cy + tmCap.tmAscent,
            toast_size_.cx - margin_.cx,
            toast_size_.cy - margin_.cy
        };

        SelectFont(hdc_, bodyFont);
        SetTextColor(hdc_, dimmedColor);
        DrawText(hdc_, data_->body_text.data(), (UINT)data_->body_text.length(),
                 &rc,
                 DT_LEFT | DT_WORDBREAK | DT_NOPREFIX |
                 DT_END_ELLIPSIS | DT_EDITCONTROL);
    }

    // close button
    {
        SelectFont(hdc_, captionFont);
        SetTextColor(hdc_, is_close_hot_ ? foreColor : dimmedColor);
        ExtTextOut(hdc_, closePos.x, closePos.y, 0, nullptr,
                   &close, 1, nullptr);
    }

    is_content_updated_ = true;
}

void DesktopNotificationController::Toast::Invalidate() {
    is_content_updated_ = false;
}

bool DesktopNotificationController::Toast::IsRedrawNeeded() const {
    return !is_content_updated_;
}

void DesktopNotificationController::Toast::UpdateBufferSize() {
    if (hdc_) {
        SIZE newSize;
        {
            TEXTMETRIC tmCap = {};
            HFONT font = data_->controller->GetCaptionFont();
            if (font) {
                SelectFont(hdc_, font);
                if (!GetTextMetrics(hdc_, &tmCap)) return;
            }

            TEXTMETRIC tmBody = {};
            font = data_->controller->GetBodyFont();
            if (font) {
                SelectFont(hdc_, font);
                if (!GetTextMetrics(hdc_, &tmBody)) return;
            }

            this->margin_ = { tmCap.tmAveCharWidth * 2, tmCap.tmAscent / 2 };

            newSize.cx = margin_.cx + (32 * tmCap.tmAveCharWidth) + margin_.cx;
            newSize.cy = margin_.cy + (tmCap.tmHeight) + margin_.cy;

            if (!data_->body_text.empty())
                newSize.cy += margin_.cy + (3 * tmBody.tmHeight);

            if (data_->image) {
                BITMAP bm;
                if (GetObject(data_->image, sizeof(bm), &bm)) {
                    // cap the image size
                    const int maxDimSize = 80;

                    auto width = bm.bmWidth;
                    auto height = bm.bmHeight;
                    if (width < height) {
                        if (height > maxDimSize) {
                            width = width * maxDimSize / height;
                            height = maxDimSize;
                        }
                    }
                    else {
                        if (width > maxDimSize) {
                            height = height * maxDimSize / width;
                            width = maxDimSize;
                        }
                    }

                    ScreenMetrics scr;
                    SIZE imageDrawSize = { scr.X(width), scr.Y(height) };

                    newSize.cx += imageDrawSize.cx + margin_.cx;

                    auto heightWithImage =
                        margin_.cy + (imageDrawSize.cy) + margin_.cy;
                    if (newSize.cy < heightWithImage)
                        newSize.cy = heightWithImage;

                    UpdateScaledImage(imageDrawSize);
                }
            }
        }

        if (newSize.cx != this->toast_size_.cx ||
           newSize.cy != this->toast_size_.cy) {
            HDC hdcScreen = GetDC(NULL);
            auto newBitmap = CreateCompatibleBitmap(hdcScreen,
                                                    newSize.cx, newSize.cy);
            ReleaseDC(NULL, hdcScreen);

            if (newBitmap) {
                if (SelectBitmap(hdc_, newBitmap)) {
                    RECT dirty1 = {}, dirty2 = {};
                    if (toast_size_.cx < newSize.cx) {
                        dirty1 = { toast_size_.cx, 0,
                                   newSize.cx, toast_size_.cy };
                    }
                    if (toast_size_.cy < newSize.cy) {
                        dirty2 = { 0, toast_size_.cy,
                                   newSize.cx, newSize.cy };
                    }

                    if (this->bitmap_) DeleteBitmap(this->bitmap_);
                    this->bitmap_ = newBitmap;
                    this->toast_size_ = newSize;

                    Invalidate();

                    // Resize also the DWM buffer to prevent flicker during
                    // window resizing. Make sure any existing data is not
                    // overwritten by marking the dirty region.
                    {
                        POINT origin = { 0, 0 };

                        UPDATELAYEREDWINDOWINFO ulw;
                        ulw.cbSize = sizeof(ulw);
                        ulw.hdcDst = NULL;
                        ulw.pptDst = nullptr;
                        ulw.psize = &toast_size_;
                        ulw.hdcSrc = hdc_;
                        ulw.pptSrc = &origin;
                        ulw.crKey = 0;
                        ulw.pblend = nullptr;
                        ulw.dwFlags = 0;
                        ulw.prcDirty = &dirty1;
                        auto b1 = UpdateLayeredWindowIndirect(hwnd_, &ulw);
                        ulw.prcDirty = &dirty2;
                        auto b2 = UpdateLayeredWindowIndirect(hwnd_, &ulw);
                        _ASSERT(b1 && b2);
                    }

                    return;
                }

                DeleteBitmap(newBitmap);
            }
        }
    }
}

void DesktopNotificationController::Toast::UpdateScaledImage(const SIZE& size) {
    BITMAP bm;
    if (!GetObject(scaled_image_, sizeof(bm), &bm) ||
       bm.bmWidth != size.cx ||
       bm.bmHeight != size.cy) {
        if (scaled_image_) DeleteBitmap(scaled_image_);
        scaled_image_ = StretchBitmap(data_->image, size.cx, size.cy);
    }
}

void DesktopNotificationController::Toast::UpdateContents() {
    Draw();

    if (IsWindowVisible(hwnd_)) {
        RECT rc;
        GetWindowRect(hwnd_, &rc);
        POINT origin = { 0, 0 };
        SIZE size = { rc.right - rc.left, rc.bottom - rc.top };
        UpdateLayeredWindow(hwnd_, NULL, nullptr, &size,
                            hdc_, &origin, 0, nullptr, 0);
    }
}

void DesktopNotificationController::Toast::Dismiss() {
    if (!is_non_interactive_) {
        // Set a flag to prevent further interaction. We don't disable the HWND
        // because we still want to receive mouse move messages in order to keep
        // the toast under the cursor and not collapse it while dismissing.
        is_non_interactive_ = true;

        AutoDismiss();
    }
}

void DesktopNotificationController::Toast::AutoDismiss() {
    KillTimer(hwnd_, TimerID_AutoDismiss);
    StartEaseOut();
}

void DesktopNotificationController::Toast::CancelDismiss() {
    KillTimer(hwnd_, TimerID_AutoDismiss);
    ease_out_active_ = false;
    ease_out_pos_ = 0;
}

void DesktopNotificationController::Toast::ScheduleDismissal() {
    SetTimer(hwnd_, TimerID_AutoDismiss, 4000, nullptr);
}

void DesktopNotificationController::Toast::ResetContents() {
    if (scaled_image_) {
        DeleteBitmap(scaled_image_);
        scaled_image_ = NULL;
    }

    Invalidate();
}

void DesktopNotificationController::Toast::PopUp(int y) {
    vertical_pos_target_ = vertical_pos_ = y;
    StartEaseIn();
}

void DesktopNotificationController::Toast::SetVerticalPosition(int y) {
    // Don't restart animation if current target is the same
    if (y == vertical_pos_target_)
        return;

    // Make sure the new animation's origin is at the current position
    vertical_pos_ += static_cast<int>(
        (vertical_pos_target_ - vertical_pos_) * stack_collapse_pos_);

    // Set new target position and start the animation
    vertical_pos_target_ = y;
    stack_collapse_start_ = GetTickCount();
    data_->controller->StartAnimation();
}

HDWP DesktopNotificationController::Toast::Animate(
    HDWP hdwp, const POINT& origin) {
    UpdateBufferSize();

    if (IsRedrawNeeded())
        Draw();

    POINT srcOrigin = { 0, 0 };

    UPDATELAYEREDWINDOWINFO ulw;
    ulw.cbSize = sizeof(ulw);
    ulw.hdcDst = NULL;
    ulw.pptDst = nullptr;
    ulw.psize = nullptr;
    ulw.hdcSrc = hdc_;
    ulw.pptSrc = &srcOrigin;
    ulw.crKey = 0;
    ulw.pblend = nullptr;
    ulw.dwFlags = 0;
    ulw.prcDirty = nullptr;

    POINT pt = { 0, 0 };
    SIZE size = { 0, 0 };
    BLENDFUNCTION blend;
    UINT dwpFlags = SWP_NOACTIVATE | SWP_SHOWWINDOW |
                    SWP_NOREDRAW | SWP_NOCOPYBITS;

    auto easeInPos = AnimateEaseIn();
    auto easeOutPos = AnimateEaseOut();
    auto stackCollapsePos = AnimateStackCollapse();

    auto yOffset = (vertical_pos_target_ - vertical_pos_) * stackCollapsePos;

    size.cx = static_cast<int>(toast_size_.cx * easeInPos);
    size.cy = toast_size_.cy;

    pt.x = origin.x - size.cx;
    pt.y = static_cast<int>(origin.y - vertical_pos_ - yOffset - size.cy);

    ulw.pptDst = &pt;
    ulw.psize = &size;

    if (ease_in_active_ && easeInPos == 1.0f) {
        ease_in_active_ = false;
        ScheduleDismissal();
    }

    this->ease_in_pos_ = easeInPos;
    this->stack_collapse_pos_ = stackCollapsePos;

    if (easeOutPos != this->ease_out_pos_) {
        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.SourceConstantAlpha = (BYTE)(255 * (1.0f - easeOutPos));
        blend.AlphaFormat = 0;

        ulw.pblend = &blend;
        ulw.dwFlags = ULW_ALPHA;

        this->ease_out_pos_ = easeOutPos;

        if (easeOutPos == 1.0f) {
            ease_out_active_ = false;

            dwpFlags &= ~SWP_SHOWWINDOW;
            dwpFlags |= SWP_HIDEWINDOW;
        }
    }

    if (stackCollapsePos == 1.0f) {
        vertical_pos_ = vertical_pos_target_;
    }

    // `UpdateLayeredWindowIndirect` updates position, size, and transparency.
    // `DeferWindowPos` updates z-order, and also position and size in case
    // ULWI fails, which can happen when one of the dimensions is zero (e.g.
    // at the beginning of ease-in).

    auto ulwResult = UpdateLayeredWindowIndirect(hwnd_, &ulw);
    hdwp = DeferWindowPos(hdwp, hwnd_, HWND_TOPMOST,
                          pt.x, pt.y, size.cx, size.cy, dwpFlags);
    return hdwp;
}

void DesktopNotificationController::Toast::StartEaseIn() {
    _ASSERT(!ease_in_active_);
    ease_in_start_ = GetTickCount();
    ease_in_active_ = true;
    data_->controller->StartAnimation();
}

void DesktopNotificationController::Toast::StartEaseOut() {
    _ASSERT(!ease_out_active_);
    ease_out_start_ = GetTickCount();
    ease_out_active_ = true;
    data_->controller->StartAnimation();
}

bool DesktopNotificationController::Toast::IsStackCollapseActive() const {
    return (vertical_pos_ != vertical_pos_target_);
}

float DesktopNotificationController::Toast::AnimateEaseIn() {
    if (!ease_in_active_)
        return ease_in_pos_;

    constexpr float duration = 500.0f;
    float elapsed = GetTickCount() - ease_in_start_;
    float time = std::min(duration, elapsed) / duration;

    // decelerating exponential ease
    const float a = -8.0f;
    auto pos = (std::exp(a * time) - 1.0f) / (std::exp(a) - 1.0f);

    return pos;
}

float DesktopNotificationController::Toast::AnimateEaseOut() {
    if (!ease_out_active_)
        return ease_out_pos_;

    constexpr float duration = 120.0f;
    float elapsed = GetTickCount() - ease_out_start_;
    float time = std::min(duration, elapsed) / duration;

    // accelerating circle ease
    auto pos = 1.0f - std::sqrt(1 - time * time);

    return pos;
}

float DesktopNotificationController::Toast::AnimateStackCollapse() {
    if (!IsStackCollapseActive())
        return stack_collapse_pos_;

    constexpr float duration = 500.0f;
    float elapsed = GetTickCount() - stack_collapse_start_;
    float time = std::min(duration, elapsed) / duration;

    // decelerating exponential ease
    const float a = -8.0f;
    auto pos = (std::exp(a * time) - 1.0f) / (std::exp(a) - 1.0f);

    return pos;
}

}
