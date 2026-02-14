// Copyright (c) 2025 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_ACTIVATOR_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_ACTIVATOR_H_

#include <NotificationActivationCallback.h>
#include <windows.h>
#include <wrl/implements.h>
#include <string>
#include <vector>

namespace electron {

class NotificationPresenterWin;

class NotificationActivator
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
          INotificationActivationCallback> {
 public:
  NotificationActivator();
  ~NotificationActivator() override;

  IFACEMETHODIMP Activate(LPCWSTR app_user_model_id,
                          LPCWSTR invoked_args,
                          const NOTIFICATION_USER_INPUT_DATA* data,
                          ULONG data_count) override;

  static void RegisterActivator();
  static void UnregisterActivator();

 private:
  static DWORD g_cookie_;
  static bool g_registered_;
};

struct ActivationUserInput {
  std::wstring key;
  std::wstring value;
};

void HandleToastActivation(const std::wstring& invoked_args,
                           std::vector<ActivationUserInput> inputs);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_ACTIVATOR_H_
