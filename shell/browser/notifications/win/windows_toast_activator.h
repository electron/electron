// Copyright (c) 2025 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_ACTIVATOR_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_ACTIVATOR_H_

#include <NotificationActivationCallback.h>
#include <windows.h>
#include <wrl/implements.h>
#include <functional>
#include <map>
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

// Arguments from a notification activation when the app was launched cold
// (no existing notification object to receive the event)
struct ActivationArguments {
  ActivationArguments();
  ~ActivationArguments();
  ActivationArguments(const ActivationArguments&);
  ActivationArguments& operator=(const ActivationArguments&);

  std::string type;       // "click", "action", or "reply"
  int action_index = -1;  // For action type, the button index
  std::string reply;      // For reply type, the user's reply text
  std::string arguments;  // Raw activation arguments
  std::map<std::string, std::string> user_inputs;  // All user inputs
};

void HandleToastActivation(const std::wstring& invoked_args,
                           std::vector<ActivationUserInput> inputs);

// Callback type for launch activation handler
using ActivationCallback = std::function<void(const ActivationArguments&)>;

// Set a callback to handle notification activation.
// If details already exist, callback is invoked immediately.
// Callback remains registered for all future activations.
void SetActivationHandler(ActivationCallback callback);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_ACTIVATOR_H_
