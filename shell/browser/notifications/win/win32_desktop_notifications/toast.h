// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_DESKTOP_NOTIFICATIONS_TOAST_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_DESKTOP_NOTIFICATIONS_TOAST_H_

#include <memory>

#include "shell/browser/notifications/win/win32_desktop_notifications/desktop_notification_controller.h"

#include "base/check.h"

namespace electron {

class DesktopNotificationController::Toast {
 public:
  static void Register(HINSTANCE hinstance);
  static HWND Create(HINSTANCE hinstance,
                     std::shared_ptr<NotificationData> data);
  static Toast* Get(HWND hwnd) {
    return reinterpret_cast<Toast*>(GetWindowLongPtr(hwnd, 0));
  }

  static LRESULT CALLBACK WndProc(HWND hwnd,
                                  UINT message,
                                  WPARAM wparam,
                                  LPARAM lparam);

  const std::shared_ptr<NotificationData>& GetNotification() const {
    return data_;
  }

  void ResetContents();

  void Dismiss();

  void PopUp(int y);
  void SetVerticalPosition(int y);
  int GetVerticalPosition() const { return vertical_pos_target_; }
  int GetHeight() const { return toast_size_.cy; }
  HDWP Animate(HDWP hdwp, const POINT& origin);
  bool IsAnimationActive() const {
    return ease_in_active_ || ease_out_active_ || IsStackCollapseActive();
  }
  bool IsHighlighted() const {
    DCHECK(!(is_highlighted_ && !IsWindowVisible(hwnd_)));
    return is_highlighted_;
  }

 private:
  enum TimerID { TimerID_AutoDismiss = 1 };

  Toast(HWND hwnd, std::shared_ptr<NotificationData>* data);
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
  static const TCHAR class_name_[];

  const HWND hwnd_;
  HDC hdc_;
  HBITMAP bitmap_ = NULL;

  class UIAutomationInterface;
  UIAutomationInterface* uia_ = nullptr;

  const std::shared_ptr<NotificationData> data_;  // never null

  SIZE toast_size_ = {};
  SIZE margin_ = {};
  RECT close_button_rect_ = {};
  HBITMAP scaled_image_ = NULL;

  int vertical_pos_ = 0;
  int vertical_pos_target_ = 0;
  bool is_non_interactive_ = false;
  bool ease_in_active_ = false;
  bool ease_out_active_ = false;
  bool is_content_updated_ = false;
  bool is_highlighted_ = false;
  bool is_close_hot_ = false;
  DWORD ease_in_start_, ease_out_start_, stack_collapse_start_;
  float ease_in_pos_ = 0, ease_out_pos_ = 0, stack_collapse_pos_ = 0;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_DESKTOP_NOTIFICATIONS_TOAST_H_
