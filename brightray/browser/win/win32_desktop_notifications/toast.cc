#define NOMINMAX
#include "toast.h"
#include "common.h"
#include <algorithm>
#include <uxtheme.h>
#include <windowsx.h>

#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "uxtheme.lib")

using namespace std;

namespace brightray {

static COLORREF GetAccentColor()
{
    bool success = false;
    if(IsAppThemed())
    {
        HKEY hkey;
        if(RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\DWM"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
        {
            COLORREF color;
            DWORD type, size;
            if(RegQueryValueEx(hkey, TEXT("AccentColor"), nullptr, &type, (BYTE*)&color, &(size = sizeof(color))) == ERROR_SUCCESS && type == REG_DWORD)
            {
                // convert from RGBA
                color = RGB(GetRValue(color), GetGValue(color), GetBValue(color));
                success = true;
            }
            else if(RegQueryValueEx(hkey, TEXT("ColorizationColor"), nullptr, &type, (BYTE*)&color, &(size = sizeof(color))) == ERROR_SUCCESS && type == REG_DWORD)
            {
                // convert from BGRA
                color = RGB(GetBValue(color), GetGValue(color), GetRValue(color));
                success = true;
            }

            RegCloseKey(hkey);

            if(success) return color;
        }
    }

    return GetSysColor(COLOR_ACTIVECAPTION);
}

// Stretches a bitmap to the specified size, preserves alpha channel
static HBITMAP StretchBitmap(HBITMAP bitmap, unsigned width, unsigned height)
{
    // We use StretchBlt for the scaling, but that discards the alpha channel.
    // Therefore we first create a separate grayscale bitmap from the alpha channel,
    // scale that separately and copy it back to the scaled color bitmap.

    BITMAP bm;
    if(!GetObject(bitmap, sizeof(bm), &bm))
        return NULL;

    if(width == 0 || height == 0)
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
        alphaSrcBitmap = CreateDIBSection(NULL, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &alphaSrcBits, NULL, 0);

        if(alphaSrcBitmap)
        {
            if(GetDIBits(hdcScreen, bitmap, 0, 0, 0, (BITMAPINFO*)&bmi, DIB_RGB_COLORS) &&
               bmi.biSizeImage > 0 &&
               (bmi.biSizeImage % 4) == 0)
            {
                auto buf = (DWORD*)_aligned_malloc(bmi.biSizeImage, sizeof(DWORD));
                if(buf)
                {
                    GetDIBits(hdcScreen, bitmap, 0, bm.bmHeight, buf, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

                    BYTE* dest = (BYTE*)alphaSrcBits;
                    for(const DWORD *src = buf, *end = (DWORD*)((BYTE*)buf + bmi.biSizeImage);
                        src != end;
                        ++src, ++dest)
                    {
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

    if(alphaSrcBitmap)
    {
        BITMAPINFOHEADER bmi = { sizeof(BITMAPINFOHEADER) };
        bmi.biWidth = width;
        bmi.biHeight = height;
        bmi.biPlanes = 1;
        bmi.biBitCount = 32;
        bmi.biCompression = BI_RGB;

        void* colorBits;
        auto colorBitmap = CreateDIBSection(NULL, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &colorBits, NULL, 0);

        void* alphaBits;
        auto alphaBitmap = CreateDIBSection(NULL, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &alphaBits, NULL, 0);

        HDC hdc = CreateCompatibleDC(NULL);
        HDC hdcSrc = CreateCompatibleDC(NULL);

        if(colorBitmap && alphaBitmap && hdc && hdcSrc)
        {
            SetStretchBltMode(hdc, HALFTONE);

            // resize color channels
            SelectObject(hdc, colorBitmap);
            SelectObject(hdcSrc, bitmap);
            StretchBlt(hdc, 0, 0, width, height, hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

            // resize alpha channel
            SelectObject(hdc, alphaBitmap);
            SelectObject(hdcSrc, alphaSrcBitmap);
            StretchBlt(hdc, 0, 0, width, height, hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

            // flush before touching the bits
            GdiFlush();

            // apply the alpha channel
            auto dest = (BYTE*)colorBits;
            auto src = (const BYTE*)alphaBits;
            auto end = src + (width * height * 4);
            while(src != end)
            {
                dest[3] = src[0];
                dest += 4;
                src += 4;
            }

            // create the resulting bitmap
            resultBitmap = CreateDIBitmap(hdcScreen, &bmi, CBM_INIT, colorBits, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
        }

        if(hdcSrc) DeleteDC(hdcSrc);
        if(hdc) DeleteDC(hdc);

        if(alphaBitmap) DeleteObject(alphaBitmap);
        if(colorBitmap) DeleteObject(colorBitmap);

        DeleteObject(alphaSrcBitmap);
    }

    ReleaseDC(NULL, hdcScreen);

    return resultBitmap;
}

DesktopNotificationController::Toast::Toast(HWND hWnd, shared_ptr<NotificationData>* data) :
    hWnd(hWnd), data(*data)
{
    HDC hdcScreen = GetDC(NULL);
    hdc = CreateCompatibleDC(hdcScreen);
    ReleaseDC(NULL, hdcScreen);
}

DesktopNotificationController::Toast::~Toast()
{
    DeleteDC(hdc);
    if(bitmap) DeleteBitmap(bitmap);
    if(scaledImage) DeleteBitmap(scaledImage);
}

void DesktopNotificationController::Toast::Register(HINSTANCE hInstance)
{
    WNDCLASSEX wc = { sizeof(wc) };
    wc.lpfnWndProc = &Toast::WndProc;
    wc.lpszClassName = className;
    wc.cbWndExtra = sizeof(Toast*);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wc);
}

LRESULT DesktopNotificationController::Toast::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_CREATE:
        {
            auto& cs = reinterpret_cast<const CREATESTRUCT*&>(lParam);
            auto inst = new Toast(hWnd, static_cast<shared_ptr<NotificationData>*>(cs->lpCreateParams));
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
        if(wParam == TimerID_AutoDismiss)
        {
            Get(hWnd)->AutoDismiss();
        }
        return 0;

    case WM_LBUTTONDOWN:
        {
            auto inst = Get(hWnd);

            inst->Dismiss();

            Notification notification(inst->data);
            if(inst->isCloseHot)
                inst->data->controller->OnNotificationDismissed(notification);
            else
                inst->data->controller->OnNotificationClicked(notification);
        }
        return 0;

    case WM_MOUSEMOVE:
        {
            auto inst = Get(hWnd);
            if(!inst->isHighlighted)
            {
                inst->isHighlighted = true;

                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd };
                TrackMouseEvent(&tme);
            }

            POINT cursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            inst->isCloseHot = (PtInRect(&inst->closeButtonRect, cursor) != FALSE);

            if(!inst->isNonInteractive)
                inst->CancelDismiss();

            inst->UpdateContents();
        }
        return 0;

    case WM_MOUSELEAVE:
        {
            auto inst = Get(hWnd);
            inst->isHighlighted = false;
            inst->isCloseHot = false;
            inst->UpdateContents();

            if(!inst->easeOutActive && inst->easeInPos == 1.0f)
                inst->ScheduleDismissal();

            // Make sure stack collapse happens if needed
            inst->data->controller->StartAnimation();
        }
        return 0;

    case WM_WINDOWPOSCHANGED:
        {
            auto& wp = reinterpret_cast<WINDOWPOS*&>(lParam);
            if(wp->flags & SWP_HIDEWINDOW)
            {
                if(!IsWindowVisible(hWnd))
                    Get(hWnd)->isHighlighted = false;
            }
        }
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND DesktopNotificationController::Toast::Create(HINSTANCE hInstance, shared_ptr<NotificationData>& data)
{
    return CreateWindowEx(WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOPMOST, className, nullptr, WS_POPUP, 0, 0, 0, 0, NULL, NULL, hInstance, &data);
}

void DesktopNotificationController::Toast::Draw()
{
    const COLORREF accent = GetAccentColor();

    COLORREF backColor;
    {
        // base background color is 2/3 of accent
        // highlighted adds a bit of intensity to every channel

        int h = isHighlighted ? (0xff / 20) : 0;

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
        // based on the lightness of background, we draw foreground in light or dark
        // shades of gray blended onto the background with slight transparency
        // to avoid sharp contrast

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

        RECT rc = { 0, 0, toastSize.cx, toastSize.cy };
        FillRect(hdc, &rc, brush);

        DeleteBrush(brush);
    }

    SetBkMode(hdc, TRANSPARENT);

    const auto close = L'\x2715';
    auto captionFont = data->controller->GetCaptionFont();
    auto bodyFont = data->controller->GetBodyFont();

    TEXTMETRIC tmCap;
    SelectFont(hdc, captionFont);
    GetTextMetrics(hdc, &tmCap);

    auto textOffsetX = margin.cx;

    BITMAP imageInfo = {};
    if(scaledImage)
    {
        GetObject(scaledImage, sizeof(imageInfo), &imageInfo);

        textOffsetX += margin.cx + imageInfo.bmWidth;
    }

    // calculate close button rect
    POINT closePos;
    {
        SIZE extent = {};
        GetTextExtentPoint32W(hdc, &close, 1, &extent);

        closeButtonRect.right = toastSize.cx;
        closeButtonRect.top = 0;

        closePos.x = closeButtonRect.right - margin.cy - extent.cx;
        closePos.y = closeButtonRect.top + margin.cy;

        closeButtonRect.left = closePos.x - margin.cy;
        closeButtonRect.bottom = closePos.y + extent.cy + margin.cy;
    }

    // image
    if(scaledImage)
    {
        HDC hdcImage = CreateCompatibleDC(NULL);
        SelectBitmap(hdcImage, scaledImage);
        BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
        AlphaBlend(hdc, margin.cx, margin.cy, imageInfo.bmWidth, imageInfo.bmHeight, hdcImage, 0, 0, imageInfo.bmWidth, imageInfo.bmHeight, blend);
        DeleteDC(hdcImage);
    }

    // caption
    {
        RECT rc = {
            textOffsetX,
            margin.cy,
            closeButtonRect.left,
            toastSize.cy
        };

        SelectFont(hdc, captionFont);
        SetTextColor(hdc, foreColor);
        DrawText(hdc, data->caption.data(), (UINT)data->caption.length(), &rc, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    // body text
    if(!data->bodyText.empty())
    {
        RECT rc = {
            textOffsetX,
            2 * margin.cy + tmCap.tmAscent,
            toastSize.cx - margin.cx,
            toastSize.cy - margin.cy
        };

        SelectFont(hdc, bodyFont);
        SetTextColor(hdc, dimmedColor);
        DrawText(hdc, data->bodyText.data(), (UINT)data->bodyText.length(), &rc, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS | DT_EDITCONTROL);
    }

    // close button
    {
        SelectFont(hdc, captionFont);
        SetTextColor(hdc, isCloseHot ? foreColor : dimmedColor);
        ExtTextOut(hdc, closePos.x, closePos.y, 0, nullptr, &close, 1, nullptr);
    }

    isContentUpdated = true;
}

void DesktopNotificationController::Toast::Invalidate()
{
    isContentUpdated = false;
}

bool DesktopNotificationController::Toast::IsRedrawNeeded() const
{
    return !isContentUpdated;
}

void DesktopNotificationController::Toast::UpdateBufferSize()
{
    if(hdc)
    {
        SIZE newSize;
        {
            TEXTMETRIC tmCap = {};
            HFONT font = data->controller->GetCaptionFont();
            if(font)
            {
                SelectFont(hdc, font);
                if(!GetTextMetrics(hdc, &tmCap)) return;
            }

            TEXTMETRIC tmBody = {};
            font = data->controller->GetBodyFont();
            if(font)
            {
                SelectFont(hdc, font);
                if(!GetTextMetrics(hdc, &tmBody)) return;
            }

            this->margin = { tmCap.tmAveCharWidth * 2, tmCap.tmAscent / 2 };

            newSize.cx = margin.cx + (32 * tmCap.tmAveCharWidth) + margin.cx;
            newSize.cy = margin.cy + (tmCap.tmHeight) + margin.cy;

            if(!data->bodyText.empty())
                newSize.cy += margin.cy + (3 * tmBody.tmHeight);

            if(data->image)
            {
                BITMAP bm;
                if(GetObject(data->image, sizeof(bm), &bm))
                {
                    // cap the image size
                    const int maxDimSize = 80;

                    auto width = bm.bmWidth;
                    auto height = bm.bmHeight;
                    if(width < height)
                    {
                        if(height > maxDimSize)
                        {
                            width = width * maxDimSize / height;
                            height = maxDimSize;
                        }
                    }
                    else
                    {
                        if(width > maxDimSize)
                        {
                            height = height * maxDimSize / width;
                            width = maxDimSize;
                        }
                    }

                    ScreenMetrics scr;
                    SIZE imageDrawSize = { scr.X(width), scr.Y(height) };

                    newSize.cx += imageDrawSize.cx + margin.cx;

                    auto heightWithImage = margin.cy + (imageDrawSize.cy) + margin.cy;
                    if(newSize.cy < heightWithImage) newSize.cy = heightWithImage;

                    UpdateScaledImage(imageDrawSize);
                }
            }
        }

        if(newSize.cx != this->toastSize.cx || newSize.cy != this->toastSize.cy)
        {
            HDC hdcScreen = GetDC(NULL);
            auto newBitmap = CreateCompatibleBitmap(hdcScreen, newSize.cx, newSize.cy);
            ReleaseDC(NULL, hdcScreen);

            if(newBitmap)
            {
                if(SelectBitmap(hdc, newBitmap))
                {
                    RECT dirty1 = {}, dirty2 = {};
                    if(toastSize.cx < newSize.cx) dirty1 = { toastSize.cx, 0, newSize.cx, toastSize.cy };
                    if(toastSize.cy < newSize.cy) dirty2 = { 0, toastSize.cy, newSize.cx, newSize.cy };

                    if(this->bitmap) DeleteBitmap(this->bitmap);
                    this->bitmap = newBitmap;
                    this->toastSize = newSize;

                    Invalidate();

                    // Resize also the DWM buffer to prevent flicker during window resizing.
                    // Make sure any existing data is not overwritten by marking the dirty region.
                    {
                        POINT origin = { 0, 0 };

                        UPDATELAYEREDWINDOWINFO ulw;
                        ulw.cbSize = sizeof(ulw);
                        ulw.hdcDst = NULL;
                        ulw.pptDst = nullptr;
                        ulw.psize = &toastSize;
                        ulw.hdcSrc = hdc;
                        ulw.pptSrc = &origin;
                        ulw.crKey = 0;
                        ulw.pblend = nullptr;
                        ulw.dwFlags = 0;
                        ulw.prcDirty = &dirty1;
                        auto b1 = UpdateLayeredWindowIndirect(hWnd, &ulw);
                        ulw.prcDirty = &dirty2;
                        auto b2 = UpdateLayeredWindowIndirect(hWnd, &ulw);
                        _ASSERT(b1 && b2);
                    }

                    return;
                }

                DeleteBitmap(newBitmap);
            }
        }
    }
}

void DesktopNotificationController::Toast::UpdateScaledImage(const SIZE& size)
{
    BITMAP bm;
    if(!GetObject(scaledImage, sizeof(bm), &bm) ||
       bm.bmWidth != size.cx ||
       bm.bmHeight != size.cy)
    {
        if(scaledImage) DeleteBitmap(scaledImage);
        scaledImage = StretchBitmap(data->image, size.cx, size.cy);
    }
}

void DesktopNotificationController::Toast::UpdateContents()
{
    Draw();

    if(IsWindowVisible(hWnd))
    {
        RECT rc;
        GetWindowRect(hWnd, &rc);
        POINT origin = { 0, 0 };
        SIZE size = { rc.right - rc.left, rc.bottom - rc.top };
        UpdateLayeredWindow(hWnd, NULL, nullptr, &size, hdc, &origin, 0, nullptr, 0);
    }
}

void DesktopNotificationController::Toast::Dismiss()
{
    if(!isNonInteractive)
    {
        // Set a flag to prevent further interaction. We don't disable the HWND because
        // we still want to receive mouse move messages in order to keep the toast under
        // the cursor and not collapse it while dismissing.
        isNonInteractive = true;

        AutoDismiss();
    }
}

void DesktopNotificationController::Toast::AutoDismiss()
{
    KillTimer(hWnd, TimerID_AutoDismiss);
    StartEaseOut();
}

void DesktopNotificationController::Toast::CancelDismiss()
{
    KillTimer(hWnd, TimerID_AutoDismiss);
    easeOutActive = false;
    easeOutPos = 0;
}

void DesktopNotificationController::Toast::ScheduleDismissal()
{
    SetTimer(hWnd, TimerID_AutoDismiss, 4000, nullptr);
}

void DesktopNotificationController::Toast::ResetContents()
{
    if(scaledImage)
    {
        DeleteBitmap(scaledImage);
        scaledImage = NULL;
    }

    Invalidate();
}

void DesktopNotificationController::Toast::PopUp(int y)
{
    verticalPosTarget = verticalPos = y;
    StartEaseIn();
}

void DesktopNotificationController::Toast::SetVerticalPosition(int y)
{
    // Don't restart animation if current target is the same
    if(y == verticalPosTarget)
        return;

    // Make sure the new animation's origin is at the current position
    verticalPos += (int)((verticalPosTarget - verticalPos) * stackCollapsePos);

    // Set new target position and start the animation
    verticalPosTarget = y;
    stackCollapseStart = GetTickCount();
    data->controller->StartAnimation();
}

HDWP DesktopNotificationController::Toast::Animate(HDWP hdwp, const POINT& origin)
{
    UpdateBufferSize();

    if(IsRedrawNeeded())
        Draw();

    POINT srcOrigin = { 0, 0 };

    UPDATELAYEREDWINDOWINFO ulw;
    ulw.cbSize = sizeof(ulw);
    ulw.hdcDst = NULL;
    ulw.pptDst = nullptr;
    ulw.psize = nullptr;
    ulw.hdcSrc = hdc;
    ulw.pptSrc = &srcOrigin;
    ulw.crKey = 0;
    ulw.pblend = nullptr;
    ulw.dwFlags = 0;
    ulw.prcDirty = nullptr;

    POINT pt = { 0, 0 };
    SIZE size = { 0, 0 };
    BLENDFUNCTION blend;
    UINT dwpFlags = SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW | SWP_NOCOPYBITS;

    auto easeInPos = AnimateEaseIn();
    auto easeOutPos = AnimateEaseOut();
    auto stackCollapsePos = AnimateStackCollapse();

    auto yOffset = (verticalPosTarget - verticalPos) * stackCollapsePos;

    size.cx = (int)(toastSize.cx * easeInPos);
    size.cy = toastSize.cy;

    pt.x = origin.x - size.cx;
    pt.y = (int)(origin.y - verticalPos - yOffset - size.cy);

    ulw.pptDst = &pt;
    ulw.psize = &size;

    if(easeInActive && easeInPos == 1.0f)
    {
        easeInActive = false;
        ScheduleDismissal();
    }

    this->easeInPos = easeInPos;
    this->stackCollapsePos = stackCollapsePos;

    if(easeOutPos != this->easeOutPos)
    {
        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.SourceConstantAlpha = (BYTE)(255 * (1.0f - easeOutPos));
        blend.AlphaFormat = 0;

        ulw.pblend = &blend;
        ulw.dwFlags = ULW_ALPHA;

        this->easeOutPos = easeOutPos;

        if(easeOutPos == 1.0f)
        {
            easeOutActive = false;

            dwpFlags &= ~SWP_SHOWWINDOW;
            dwpFlags |= SWP_HIDEWINDOW;
        }
    }

    if(stackCollapsePos == 1.0f)
    {
        verticalPos = verticalPosTarget;
    }

    // `UpdateLayeredWindowIndirect` updates position, size, and transparency.
    // `DeferWindowPos` updates z-order, and also position and size in case ULWI fails,
    // which can happen when one of the dimensions is zero (e.g. at the beginning of ease-in).

    auto ulwResult = UpdateLayeredWindowIndirect(hWnd, &ulw);
    hdwp = DeferWindowPos(hdwp, hWnd, HWND_TOPMOST, pt.x, pt.y, size.cx, size.cy, dwpFlags);
    return hdwp;
}

void DesktopNotificationController::Toast::StartEaseIn()
{
    _ASSERT(!easeInActive);
    easeInStart = GetTickCount();
    easeInActive = true;
    data->controller->StartAnimation();
}

void DesktopNotificationController::Toast::StartEaseOut()
{
    _ASSERT(!easeOutActive);
    easeOutStart = GetTickCount();
    easeOutActive = true;
    data->controller->StartAnimation();
}

bool DesktopNotificationController::Toast::IsStackCollapseActive() const
{
    return (verticalPos != verticalPosTarget);
}

float DesktopNotificationController::Toast::AnimateEaseIn()
{
    if(!easeInActive)
        return easeInPos;

    constexpr float duration = 500.0f;
    float time = std::min(duration, (float)(GetTickCount() - easeInStart)) / duration;

    // decelerating exponential ease
    const float a = -8.0f;
    auto pos = (std::exp(a * time) - 1.0f) / (std::exp(a) - 1.0f);

    return pos;
}

float DesktopNotificationController::Toast::AnimateEaseOut()
{
    if(!easeOutActive)
        return easeOutPos;

    constexpr float duration = 120.0f;
    float time = std::min(duration, (float)(GetTickCount() - easeOutStart)) / duration;

    // accelerating circle ease
    auto pos = 1.0f - std::sqrt(1 - time * time);

    return pos;
}

float DesktopNotificationController::Toast::AnimateStackCollapse()
{
    if(!IsStackCollapseActive())
        return stackCollapsePos;

    constexpr float duration = 500.0f;
    float time = std::min(duration, (float)(GetTickCount() - stackCollapseStart)) / duration;

    // decelerating exponential ease
    const float a = -8.0f;
    auto pos = (std::exp(a * time) - 1.0f) / (std::exp(a) - 1.0f);

    return pos;
}

}
