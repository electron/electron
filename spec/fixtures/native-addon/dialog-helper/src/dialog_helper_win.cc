#include "dialog_helper.h"

#include <windows.h>

#include <commctrl.h>

namespace {

// Electron assigns custom TaskDialog button IDs starting at this value; a
// button at index |i| has ID kIDStart + i. Keep in sync with
// shell/browser/ui/message_box_win.cc.
constexpr int kIDStart = 100;

struct FindContext {
  DWORD process_id;
  HWND result;
};

BOOL CALLBACK FindMessageBoxProc(HWND hwnd, LPARAM param) {
  auto* context = reinterpret_cast<FindContext*>(param);
  if (!::IsWindowVisible(hwnd))
    return TRUE;

  // Task dialogs (and other common dialogs) use the same window class.
  wchar_t class_name[16] = {0};
  ::GetClassNameW(hwnd, class_name, 16);
  if (::lstrcmpW(class_name, L"#32770") != 0)
    return TRUE;

  DWORD process_id = 0;
  ::GetWindowThreadProcessId(hwnd, &process_id);
  if (process_id != context->process_id)
    return TRUE;

  context->result = hwnd;
  return FALSE;
}

// Finds the visible task dialog owned by the current process.
HWND FindMessageBox() {
  FindContext context = {::GetCurrentProcessId(), nullptr};
  ::EnumWindows(&FindMessageBoxProc, reinterpret_cast<LPARAM>(&context));
  return context.result;
}

}  // namespace

namespace dialog_helper {

DialogInfo GetDialogInfo(char*, size_t) {
  DialogInfo info;
  info.type = FindMessageBox() ? "message-box" : "none";
  return info;
}

bool ClickMessageBoxButton(char*, size_t, int button_index) {
  HWND window = FindMessageBox();
  if (!window)
    return false;

  // TDM_CLICK_BUTTON clicks the button whose command ID is in wParam. Passing
  // an index that maps to an ID Electron never created (e.g. a large index)
  // simulates an external process clicking a nonexistent button, which some
  // software triggers by enumerating dialog windows.
  ::SendMessageW(window, TDM_CLICK_BUTTON,
                 static_cast<WPARAM>(kIDStart + button_index), 0);
  return true;
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

}  // namespace dialog_helper
