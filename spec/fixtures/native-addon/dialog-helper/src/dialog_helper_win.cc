#include "dialog_helper.h"

#include <windows.h>

#include <commctrl.h>

#include <cstring>

namespace {

// Electron assigns custom TaskDialog button IDs starting at this value; a
// button at index |i| has ID kIDStart + i. Keep in sync with
// shell/browser/ui/message_box_win.cc.
constexpr int kIDStart = 100;

// Extract the HWND from the native handle buffer. The buffer contains the
// HWND returned by BrowserWindow.getNativeWindowHandle(), which is the same
// window that message_box_win.cc passes to TaskDialogIndirect as hwndParent,
// i.e. the owner of the task dialog.
HWND GetHWNDFromHandle(char* handle, size_t size) {
  if (!handle || size != sizeof(HWND))
    return nullptr;
  HWND hwnd = nullptr;
  std::memcpy(&hwnd, handle, sizeof(HWND));
  return hwnd;
}

struct FindContext {
  DWORD process_id;
  HWND owner;
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

  // Only match the dialog owned by the given window, so that unrelated
  // dialog-class windows in the process (e.g. a dialog leaked by an earlier
  // test) are never picked up.
  if (::GetWindow(hwnd, GW_OWNER) != context->owner)
    return TRUE;

  context->result = hwnd;
  return FALSE;
}

// Finds the visible task dialog created by the current process that is owned
// by the window identified by the handle buffer.
HWND FindMessageBox(char* handle, size_t size) {
  HWND owner = GetHWNDFromHandle(handle, size);
  if (!owner)
    return nullptr;
  FindContext context = {::GetCurrentProcessId(), owner, nullptr};
  ::EnumWindows(&FindMessageBoxProc, reinterpret_cast<LPARAM>(&context));
  return context.result;
}

}  // namespace

namespace dialog_helper {

DialogInfo GetDialogInfo(char* handle, size_t size) {
  DialogInfo info;
  info.type = FindMessageBox(handle, size) ? "message-box" : "none";
  return info;
}

bool ClickMessageBoxButton(char* handle, size_t size, int button_index) {
  HWND window = FindMessageBox(handle, size);
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
