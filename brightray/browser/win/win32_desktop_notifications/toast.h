#pragma once
#include "desktop_notification_controller.h"

namespace brightray {

class DesktopNotificationController::Toast
{
public:
    static void Register(HINSTANCE hInstance);
    static HWND Create(HINSTANCE hInstance, std::shared_ptr<NotificationData>& data);
    static Toast* Get(HWND hWnd)
    {
        return reinterpret_cast<Toast*>(GetWindowLongPtr(hWnd, 0));
    }

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    const std::shared_ptr<NotificationData>& GetNotification() const
    {
        return data;
    }

    void ResetContents();

    void Dismiss();

    void PopUp(int y);
    void SetVerticalPosition(int y);
    int GetVerticalPosition() const
    {
        return verticalPosTarget;
    }
    int GetHeight() const
    {
        return toastSize.cy;
    }
    HDWP Animate(HDWP hdwp, const POINT& origin);
    bool IsAnimationActive() const
    {
        return easeInActive || easeOutActive || IsStackCollapseActive();
    }
    bool IsHighlighted() const
    {
        _ASSERT(!(isHighlighted && !IsWindowVisible(hWnd)));
        return isHighlighted;
    }

private:
    enum TimerID {
        TimerID_AutoDismiss = 1
    };

    Toast(HWND hWnd, std::shared_ptr<NotificationData>* data);
    ~Toast();

    void UpdateBufferSize();
    void UpdateScaledImage(const SIZE& size);
    void Draw();
    void Invalidate();
    bool IsRedrawNeeded() const;
    void UpdateContents();

    void AutoDismiss();
    void CancelDismiss();
    void ScheduleDismissal();

    void StartEaseIn();
    void StartEaseOut();
    bool IsStackCollapseActive() const;

    float AnimateEaseIn();
    float AnimateEaseOut();
    float AnimateStackCollapse();

private:
    static constexpr const TCHAR className[] = TEXT("DesktopNotificationToast");

    const HWND hWnd;
    HDC hdc;
    HBITMAP bitmap = NULL;

    const std::shared_ptr<NotificationData> data; // never null

    SIZE toastSize = {};
    SIZE margin = {};
    RECT closeButtonRect = {};
    HBITMAP scaledImage = NULL;

    int verticalPos = 0;
    int verticalPosTarget = 0;
    bool isNonInteractive = false;
    bool easeInActive = false;
    bool easeOutActive = false;
    bool isContentUpdated = false, isHighlighted = false, isCloseHot = false;
    DWORD easeInStart, easeOutStart, stackCollapseStart;
    float easeInPos = 0, easeOutPos = 0, stackCollapsePos = 0;
};

}
