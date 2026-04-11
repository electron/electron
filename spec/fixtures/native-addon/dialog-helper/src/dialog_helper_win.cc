#include "dialog_helper.h"

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace dialog_helper {
namespace {

constexpr wchar_t kDialogClassName[] = L"#32770";
constexpr auto kPollInterval = std::chrono::milliseconds(50);
constexpr auto kDialogTimeout = std::chrono::seconds(5);

HWND GetWindowHandle(char* handle, size_t size) {
  if (size < sizeof(HWND))
    return {};
  return *reinterpret_cast<HWND*>(handle);
}

std::string WideToUtf8(const std::wstring& input) {
  if (input.empty())
    return {};

  const int size = ::WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, nullptr,
                                         0, nullptr, nullptr);
  if (size <= 1)
    return {};

  std::string output(size, '\0');
  const int converted = ::WideCharToMultiByte(
      CP_UTF8, 0, input.c_str(), -1, output.data(), size, nullptr, nullptr);
  if (converted <= 1)
    return {};

  output.resize(converted - 1);
  return output;
}

std::wstring GetWindowText(HWND hwnd) {
  int length = ::GetWindowTextLengthW(hwnd);
  if (length <= 0)
    return {};

  std::wstring text(length + 1, L'\0');
  ::GetWindowTextW(hwnd, text.data(), length + 1);
  text.resize(length);
  return text;
}

bool IsDialogForOwner(HWND hwnd, HWND owner) {
  if (!::IsWindow(hwnd) || !::IsWindowVisible(hwnd))
    return false;
  if (::GetWindow(hwnd, GW_OWNER) != owner)
    return false;

  wchar_t class_name[32] = {0};
  if (::GetClassNameW(hwnd, class_name, std::size(class_name)) == 0)
    return false;

  return wcscmp(class_name, kDialogClassName) == 0;
}

struct FindDialogState {
  HWND owner = nullptr;
  HWND dialog = nullptr;
};

BOOL CALLBACK FindDialogProc(HWND hwnd, LPARAM lparam) {
  auto* state = reinterpret_cast<FindDialogState*>(lparam);
  if (IsDialogForOwner(hwnd, state->owner)) {
    state->dialog = hwnd;
    return FALSE;
  }
  return TRUE;
}

HWND FindOwnedDialog(HWND owner) {
  FindDialogState state{owner, nullptr};
  ::EnumWindows(&FindDialogProc, reinterpret_cast<LPARAM>(&state));
  return state.dialog;
}

bool WaitForOwnedDialog(HWND owner,
                        std::chrono::milliseconds timeout,
                        HWND* dialog) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (HWND candidate = FindOwnedDialog(owner); candidate != nullptr) {
      *dialog = candidate;
      return true;
    }
    std::this_thread::sleep_for(kPollInterval);
  }
  return false;
}

bool WaitForWindowToClose(HWND hwnd, std::chrono::milliseconds timeout) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (!::IsWindow(hwnd))
      return true;
    std::this_thread::sleep_for(kPollInterval);
  }
  return !::IsWindow(hwnd);
}

bool IsButtonControl(HWND hwnd) {
  wchar_t class_name[16] = {0};
  if (::GetClassNameW(hwnd, class_name, std::size(class_name)) == 0)
    return false;
  return wcscmp(class_name, L"Button") == 0;
}

bool IsStaticTextControl(HWND hwnd) {
  wchar_t class_name[16] = {0};
  if (::GetClassNameW(hwnd, class_name, std::size(class_name)) == 0)
    return false;
  return wcscmp(class_name, L"Static") == 0;
}

bool CompareWindowPositions(HWND left, HWND right) {
  RECT left_rect;
  RECT right_rect;
  ::GetWindowRect(left, &left_rect);
  ::GetWindowRect(right, &right_rect);
  if (left_rect.top != right_rect.top)
    return left_rect.top < right_rect.top;
  return left_rect.left < right_rect.left;
}

struct EnumChildWindowsState {
  std::vector<HWND> buttons;
};

struct TextControlState {
  std::vector<HWND> controls;
};

BOOL CALLBACK CollectButtonsProc(HWND hwnd, LPARAM lparam) {
  auto* state = reinterpret_cast<EnumChildWindowsState*>(lparam);
  if (::IsWindowVisible(hwnd) && ::IsWindowEnabled(hwnd) &&
      IsButtonControl(hwnd)) {
    LONG style = ::GetWindowLongW(hwnd, GWL_STYLE);
    if ((style & BS_TYPEMASK) != BS_CHECKBOX &&
        (style & BS_TYPEMASK) != BS_AUTOCHECKBOX &&
        (style & BS_TYPEMASK) != BS_3STATE &&
        (style & BS_TYPEMASK) != BS_AUTO3STATE) {
      state->buttons.push_back(hwnd);
    }
  }
  return TRUE;
}

std::vector<HWND> GetMessageBoxButtons(HWND dialog) {
  EnumChildWindowsState state;
  ::EnumChildWindows(dialog, &CollectButtonsProc,
                     reinterpret_cast<LPARAM>(&state));

  std::sort(state.buttons.begin(), state.buttons.end(), CompareWindowPositions);

  return state.buttons;
}

BOOL CALLBACK CollectStaticTextProc(HWND hwnd, LPARAM lparam) {
  if (!::IsWindowVisible(hwnd) || !IsStaticTextControl(hwnd))
    return TRUE;

  std::wstring text = GetWindowText(hwnd);
  if (text.empty())
    return TRUE;

  auto* state = reinterpret_cast<TextControlState*>(lparam);
  state->controls.push_back(hwnd);
  return TRUE;
}

std::vector<HWND> GetMessageBoxTextControls(HWND dialog) {
  TextControlState state;
  ::EnumChildWindows(dialog, &CollectStaticTextProc,
                     reinterpret_cast<LPARAM>(&state));
  std::sort(state.controls.begin(), state.controls.end(),
            CompareWindowPositions);
  return state.controls;
}

void PopulateMessageBoxText(HWND dialog, DialogInfo* info) {
  std::vector<HWND> text_controls = GetMessageBoxTextControls(dialog);
  if (text_controls.empty())
    return;

  info->message = WideToUtf8(GetWindowText(text_controls[0]));
  if (text_controls.size() > 1)
    info->detail = WideToUtf8(GetWindowText(text_controls[1]));
}

// NOTE: This only escapes \\ and ". Control characters (\n, \r, \t, etc.)
// are passed through unescaped, which is technically invalid JSON.
// But that's OK here in a test environment where control the label text.
std::string EscapeJsonString(const std::string& input) {
  std::string escaped;
  escaped.reserve(input.size());
  for (const char ch : input) {
    if (ch == '\\' || ch == '"')
      escaped.push_back('\\');
    escaped.push_back(ch);
  }
  return escaped;
}

std::string ButtonsToJson(const std::vector<HWND>& buttons) {
  std::string json = "[";
  for (size_t i = 0; i < buttons.size(); ++i) {
    if (i > 0)
      json += ',';
    json += '"';
    json += EscapeJsonString(WideToUtf8(GetWindowText(buttons[i])));
    json += '"';
  }
  json += "]";
  return json;
}

bool ClickButtonByIndex(HWND dialog, int button_index) {
  std::vector<HWND> buttons = GetMessageBoxButtons(dialog);
  if (button_index < 0 || static_cast<size_t>(button_index) >= buttons.size())
    return false;

  ::SendMessageW(buttons[button_index], BM_CLICK, 0, 0);
  return true;
}

bool FocusWindow(HWND hwnd) {
  if (!::IsWindow(hwnd))
    return false;

  if (::IsIconic(hwnd))
    ::ShowWindow(hwnd, SW_RESTORE);

  const HWND foreground_window = ::GetForegroundWindow();
  const DWORD foreground_thread =
      foreground_window == nullptr
          ? 0
          : ::GetWindowThreadProcessId(foreground_window, nullptr);
  const DWORD current_thread = ::GetCurrentThreadId();
  const bool attached =
      foreground_thread != 0 && foreground_thread != current_thread &&
      ::AttachThreadInput(current_thread, foreground_thread, TRUE) != FALSE;

  ::BringWindowToTop(hwnd);
  ::SetActiveWindow(hwnd);
  ::SetForegroundWindow(hwnd);

  if (attached) {
    ::AttachThreadInput(current_thread, foreground_thread, FALSE);
  }

  return ::GetForegroundWindow() == hwnd;
}

// NOTE: SendInput delivers the keystroke to the foreground window, so make
// sure the target window is re-activated first.
bool SendTabKey(HWND hwnd) {
  if (!FocusWindow(hwnd))
    return false;

  INPUT inputs[2] = {};

  inputs[0].type = INPUT_KEYBOARD;
  inputs[0].ki.wVk = VK_TAB;

  inputs[1].type = INPUT_KEYBOARD;
  inputs[1].ki.wVk = VK_TAB;
  inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

  return ::SendInput(2, inputs, sizeof(INPUT)) == 2;
}

}  // namespace

DialogInfo GetDialogInfo(char* handle, size_t size) {
  DialogInfo info;
  info.type = "none";

  const HWND owner = GetWindowHandle(handle, size);
  if (!::IsWindow(owner))
    return info;

  const HWND dialog = FindOwnedDialog(owner);
  if (dialog == nullptr)
    return info;

  info.type = "message-box";
  std::vector<HWND> buttons = GetMessageBoxButtons(dialog);
  info.buttons = ButtonsToJson(buttons);
  PopulateMessageBoxText(dialog, &info);
  return info;
}

bool ClickMessageBoxButton(char* handle, size_t size, int button_index) {
  HWND owner = GetWindowHandle(handle, size);
  if (!::IsWindow(owner))
    return false;

  HWND dialog = FindOwnedDialog(owner);
  if (dialog == nullptr)
    return false;

  return ClickButtonByIndex(dialog, button_index);
}

bool ClickCheckbox(char*, size_t) {
  return false;
}

bool CancelFileDialog(char*, size_t) {
  return false;
}

bool AcceptFileDialog(char*, size_t, const std::string&) {
  return false;
}

bool ClickMessageBoxButtonAndSendTabLater(char* handle,
                                          size_t size,
                                          int button_index,
                                          int post_close_delay_ms) {
  HWND owner = GetWindowHandle(handle, size);
  if (!::IsWindow(owner))
    return false;

  // Fire-and-forget: the thread polls for the dialog, clicks, waits for it
  // to close, then sends a Tab key. Detached because there is no teardown
  // requirement — the thread will exit on its own after the dialog closes or
  // the timeout expires.
  std::thread([owner, button_index, post_close_delay_ms]() {
    HWND dialog = nullptr;
    if (!WaitForOwnedDialog(owner, kDialogTimeout, &dialog))
      return;
    if (!ClickButtonByIndex(dialog, button_index))
      return;
    if (!WaitForWindowToClose(dialog, kDialogTimeout))
      return;
    if (post_close_delay_ms > 0) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(post_close_delay_ms));
    }
    SendTabKey(owner);
  }).detach();

  return true;
}

}  // namespace dialog_helper
