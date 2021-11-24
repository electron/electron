// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_DESKTOP_NOTIFICATIONS_DESKTOP_NOTIFICATION_CONTROLLER_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_DESKTOP_NOTIFICATIONS_DESKTOP_NOTIFICATION_CONTROLLER_H_

#include <Windows.h>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace electron {

struct NotificationData;

class DesktopNotificationController {
 public:
  explicit DesktopNotificationController(unsigned maximum_toasts = 3);
  ~DesktopNotificationController();

  class Notification;
  Notification AddNotification(std::u16string caption,
                               std::u16string body_text,
                               HBITMAP image);
  void CloseNotification(const Notification& notification);

  // Event handlers -- override to receive the events
 private:
  class Toast;
  DesktopNotificationController(const DesktopNotificationController&) = delete;

  struct ToastInstance {
    ToastInstance(HWND, std::shared_ptr<NotificationData>);
    ~ToastInstance();
    ToastInstance(ToastInstance&&);
    ToastInstance(const ToastInstance&) = delete;
    ToastInstance& operator=(ToastInstance&&) = default;

    HWND hwnd;
    std::shared_ptr<NotificationData> data;
  };

  virtual void OnNotificationClosed(const Notification& notification) {}
  virtual void OnNotificationClicked(const Notification& notification) {}
  virtual void OnNotificationDismissed(const Notification& notification) {}

  static HINSTANCE RegisterWndClasses();
  void StartAnimation();
  HFONT GetCaptionFont();
  HFONT GetBodyFont();
  void InitializeFonts();
  void ClearAssets();
  void AnimateAll();
  void CheckQueue();
  void CreateToast(std::shared_ptr<NotificationData>&& data);
  HWND GetToast(const NotificationData* data) const;
  void DestroyToast(ToastInstance* inst);

  static LRESULT CALLBACK WndProc(HWND hwnd,
                                  UINT message,
                                  WPARAM wparam,
                                  LPARAM lparam);
  static DesktopNotificationController* Get(HWND hwnd) {
    return reinterpret_cast<DesktopNotificationController*>(
        GetWindowLongPtr(hwnd, 0));
  }

  static constexpr int toast_margin_ = 20;
  static const TCHAR class_name_[];
  enum TimerID { TimerID_Animate = 1 };
  HWND hwnd_controller_ = NULL;
  HFONT caption_font_ = NULL, body_font_ = NULL;
  std::vector<ToastInstance> instances_;
  std::deque<std::shared_ptr<NotificationData>> queue_;
  bool is_animating_ = false;
};

class DesktopNotificationController::Notification {
 public:
  Notification();
  explicit Notification(const std::shared_ptr<NotificationData>& data);
  Notification(const Notification&);
  ~Notification();

  bool operator==(const Notification& other) const;

  void Close();
  void Set(std::u16string caption, std::u16string body_text, HBITMAP image);

 private:
  std::shared_ptr<NotificationData> data_;

  friend class DesktopNotificationController;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_DESKTOP_NOTIFICATIONS_DESKTOP_NOTIFICATION_CONTROLLER_H_
