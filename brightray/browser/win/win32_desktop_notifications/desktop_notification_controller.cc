#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "desktop_notification_controller.h"
#include "common.h"
#include "toast.h"
#include <algorithm>
#include <vector>
#include <windowsx.h>

using namespace std;

namespace brightray {

HBITMAP CopyBitmap(HBITMAP bitmap)
{
    HBITMAP ret = NULL;

    BITMAP bm;
    if(bitmap && GetObject(bitmap, sizeof(bm), &bm))
    {
        HDC hdcScreen = GetDC(NULL);
        ret = CreateCompatibleBitmap(hdcScreen, bm.bmWidth, bm.bmHeight);
        ReleaseDC(NULL, hdcScreen);

        if(ret)
        {
            HDC hdcSrc = CreateCompatibleDC(NULL);
            HDC hdcDst = CreateCompatibleDC(NULL);
            SelectBitmap(hdcSrc, bitmap);
            SelectBitmap(hdcDst, ret);
            BitBlt(hdcDst, 0, 0, bm.bmWidth, bm.bmHeight, hdcSrc, 0, 0, SRCCOPY);
            DeleteDC(hdcDst);
            DeleteDC(hdcSrc);
        }
    }

    return ret;
}

HINSTANCE DesktopNotificationController::RegisterWndClasses()
{
    // We keep a static `module` variable which serves a dual purpose:
    // 1. Stores the HINSTANCE where the window classes are registered, which can be passed to `CreateWindow`
    // 2. Indicates whether we already attempted the registration so that we don't do it twice (we don't retry
    //    even if registration fails, as there is no point.
    static HMODULE module = NULL;

    if(!module)
    {
        if(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&RegisterWndClasses), &module))
        {
            Toast::Register(module);

            WNDCLASSEX wc = { sizeof(wc) };
            wc.lpfnWndProc = &WndProc;
            wc.lpszClassName = className;
            wc.cbWndExtra = sizeof(DesktopNotificationController*);
            wc.hInstance = module;

            RegisterClassEx(&wc);
        }
    }

    return module;
}

DesktopNotificationController::DesktopNotificationController(unsigned maximumToasts)
{
    instances.reserve(maximumToasts);
}

DesktopNotificationController::~DesktopNotificationController()
{
    for(auto&& inst : instances) DestroyToast(inst);
    if(hwndController) DestroyWindow(hwndController);
    ClearAssets();
}

LRESULT CALLBACK DesktopNotificationController::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_CREATE:
        {
            auto& cs = reinterpret_cast<const CREATESTRUCT*&>(lParam);
            SetWindowLongPtr(hWnd, 0, (LONG_PTR)cs->lpCreateParams);
        }
        break;

    case WM_TIMER:
        if(wParam == TimerID_Animate)
        {
            Get(hWnd)->AnimateAll();
        }
        return 0;

    case WM_DISPLAYCHANGE:
        {
            auto inst = Get(hWnd);
            inst->ClearAssets();
            inst->AnimateAll();
        }
        break;

    case WM_SETTINGCHANGE:
        if(wParam == SPI_SETWORKAREA)
        {
            Get(hWnd)->AnimateAll();
        }
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void DesktopNotificationController::StartAnimation()
{
    _ASSERT(hwndController);

    if(!isAnimating && hwndController)
    {
        // NOTE: 15ms is shorter than what we'd need for 60 fps, but since the timer
        //       is not accurate we must request a higher frame rate to get at least 60

        SetTimer(hwndController, TimerID_Animate, 15, nullptr);
        isAnimating = true;
    }
}

HFONT DesktopNotificationController::GetCaptionFont()
{
    InitializeFonts();
    return captionFont;
}

HFONT DesktopNotificationController::GetBodyFont()
{
    InitializeFonts();
    return bodyFont;
}

void DesktopNotificationController::InitializeFonts()
{
    if(!bodyFont)
    {
        NONCLIENTMETRICS metrics = { sizeof(metrics) };
        if(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
        {
            auto baseHeight = metrics.lfMessageFont.lfHeight;

            HDC hdc = GetDC(NULL);
            auto dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(NULL, hdc);

            metrics.lfMessageFont.lfHeight = (LONG)ScaleForDpi(baseHeight * 1.1f, dpiY);
            bodyFont = CreateFontIndirect(&metrics.lfMessageFont);

            if(captionFont) DeleteFont(captionFont);
            metrics.lfMessageFont.lfHeight = (LONG)ScaleForDpi(baseHeight * 1.4f, dpiY);
            captionFont = CreateFontIndirect(&metrics.lfMessageFont);
        }
    }
}

void DesktopNotificationController::ClearAssets()
{
    if(captionFont) { DeleteFont(captionFont); captionFont = NULL; }
    if(bodyFont) { DeleteFont(bodyFont); bodyFont = NULL; }
}

void DesktopNotificationController::AnimateAll()
{
    // NOTE: This function refreshes position and size of all toasts according
    // to all current conditions. Animation time is only one of the variables
    // influencing them. Screen resolution is another.

    bool keepAnimating = false;

    if(!instances.empty())
    {
        RECT workArea;
        if(SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0))
        {
            ScreenMetrics metrics;
            POINT origin = { workArea.right,
                             workArea.bottom - metrics.Y(toastMargin<int>) };

            auto hdwp = BeginDeferWindowPos((int)instances.size());
            for(auto&& inst : instances)
            {
                if(!inst.hwnd) continue;

                auto notification = Toast::Get(inst.hwnd);
                hdwp = notification->Animate(hdwp, origin);
                if(!hdwp) break;
                keepAnimating |= notification->IsAnimationActive();
            }
            if(hdwp) EndDeferWindowPos(hdwp);
        }
    }

    if(!keepAnimating)
    {
        _ASSERT(hwndController);
        if(hwndController) KillTimer(hwndController, TimerID_Animate);
        isAnimating = false;
    }

    // Purge dismissed notifications and collapse the stack between
    // items which are highlighted
    if(!instances.empty())
    {
        auto isAlive = [](ToastInstance& inst) {
            return inst.hwnd && IsWindowVisible(inst.hwnd);
        };

        auto isHighlighted = [](ToastInstance& inst) {
            return inst.hwnd && Toast::Get(inst.hwnd)->IsHighlighted();
        };

        for(auto it = instances.begin();; ++it)
        {
            // find next highlighted item
            auto it2 = find_if(it, instances.end(), isHighlighted);

            // collapse the stack in front of the highlighted item
            it = stable_partition(it, it2, isAlive);

            // purge the dead items
            for_each(it, it2, [this](auto&& inst) { DestroyToast(inst); });

            if(it2 == instances.end())
            {
                instances.erase(it, it2);
                break;
            }

            it = move(it2);
        }
    }

    // Set new toast positions
    if(!instances.empty())
    {
        ScreenMetrics metrics;
        auto margin = metrics.Y(toastMargin<int>);

        int targetPos = 0;
        for(auto&& inst : instances)
        {
            if(inst.hwnd)
            {
                auto toast = Toast::Get(inst.hwnd);

                if(toast->IsHighlighted())
                    targetPos = toast->GetVerticalPosition();
                else
                    toast->SetVerticalPosition(targetPos);

                targetPos += toast->GetHeight() + margin;
            }
        }
    }

    // Create new toasts from the queue
    CheckQueue();
}

DesktopNotificationController::Notification DesktopNotificationController::AddNotification(std::wstring caption, std::wstring bodyText, HBITMAP image)
{
    NotificationLink data(this);

    data->caption = move(caption);
    data->bodyText = move(bodyText);
    data->image = CopyBitmap(image);

    // Enqueue new notification
    Notification ret = *queue.insert(queue.end(), move(data));
    CheckQueue();
    return ret;
}

void DesktopNotificationController::CloseNotification(Notification& notification)
{
    // Remove it from the queue
    auto it = find(queue.begin(), queue.end(), notification.data);
    if(it != queue.end())
    {
        queue.erase(it);
        this->OnNotificationClosed(notification);
        return;
    }

    // Dismiss active toast
    auto hwnd = GetToast(notification.data.get());
    if(hwnd)
    {
        auto toast = Toast::Get(hwnd);
        toast->Dismiss();
    }
}

void DesktopNotificationController::CheckQueue()
{
    while(instances.size() < instances.capacity() && !queue.empty())
    {
        CreateToast(move(queue.front()));
        queue.pop_front();
    }
}

void DesktopNotificationController::CreateToast(NotificationLink&& data)
{
    auto hInstance = RegisterWndClasses();
    auto hwnd = Toast::Create(hInstance, data);
    if(hwnd)
    {
        int toastPos = 0;
        if(!instances.empty())
        {
            auto& item = instances.back();
            _ASSERT(item.hwnd);

            ScreenMetrics scr;
            auto toast = Toast::Get(item.hwnd);
            toastPos = toast->GetVerticalPosition() + toast->GetHeight() + scr.Y(toastMargin<int>);
        }

        instances.push_back({ hwnd, move(data) });

        if(!hwndController)
        {
            // NOTE: We cannot use a message-only window because we need to receive system notifications
            hwndController = CreateWindow(className, nullptr, 0, 0, 0, 0, 0, NULL, NULL, hInstance, this);
        }

        auto toast = Toast::Get(hwnd);
        toast->PopUp(toastPos);
    }
}

HWND DesktopNotificationController::GetToast(const NotificationData* data) const
{
    auto it = find_if(instances.cbegin(), instances.cend(), [data](auto&& inst) {
        auto toast = Toast::Get(inst.hwnd);
        return data == toast->GetNotification().get();
    });

    return (it != instances.cend()) ? it->hwnd : NULL;
}

void DesktopNotificationController::DestroyToast(ToastInstance& inst)
{
    if(inst.hwnd)
    {
        auto data = Toast::Get(inst.hwnd)->GetNotification();

        DestroyWindow(inst.hwnd);
        inst.hwnd = NULL;

        Notification notification(data);
        OnNotificationClosed(notification);
    }
}


DesktopNotificationController::Notification::Notification(const shared_ptr<NotificationData>& data) :
    data(data)
{
    _ASSERT(data != nullptr);
}

bool DesktopNotificationController::Notification::operator==(const Notification& other) const
{
    return data == other.data;
}

void DesktopNotificationController::Notification::Close()
{
    // No business calling this when not pointing to a valid instance
    _ASSERT(data);

    if(data->controller)
        data->controller->CloseNotification(*this);
}

void DesktopNotificationController::Notification::Set(std::wstring caption, std::wstring bodyText, HBITMAP image)
{
    // No business calling this when not pointing to a valid instance
    _ASSERT(data);

    // Do nothing when the notification has been closed
    if(!data->controller)
        return;

    if(data->image) DeleteBitmap(data->image);

    data->caption = move(caption);
    data->bodyText = move(bodyText);
    data->image = CopyBitmap(image);

    auto hwnd = data->controller->GetToast(data.get());
    if(hwnd)
    {
        auto toast = Toast::Get(hwnd);
        toast->ResetContents();
    }

    // Change of contents can affect size and position of all toasts
    data->controller->StartAnimation();
}


DesktopNotificationController::NotificationLink::NotificationLink(DesktopNotificationController* controller) :
    shared_ptr(make_shared<NotificationData>())
{
    get()->controller = controller;
}

DesktopNotificationController::NotificationLink::~NotificationLink()
{
    auto p = get();
    if(p) p->controller = nullptr;
}

}
