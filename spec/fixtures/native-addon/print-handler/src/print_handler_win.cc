#include "print_handler.h"

#include <windows.h>

#include <atomic>
#include <thread>

namespace {

std::atomic<bool> g_watching{false};
std::atomic<bool> g_dismissed{false};
std::atomic<bool> g_should_print{false};
std::thread g_thread;

// EnumWindows callback — looks for a visible dialog box (#32770) belonging
// to the current process.  In the test environment the only dialog that
// appears after webContents.print() is the system print dialog shown by
// PrintDlgEx().
BOOL CALLBACK FindPrintDialog(HWND hwnd, LPARAM lParam) {
  // Must be visible.
  if (!IsWindowVisible(hwnd))
    return TRUE;

  // Must belong to our process.
  DWORD pid = 0;
  GetWindowThreadProcessId(hwnd, &pid);
  if (pid != GetCurrentProcessId())
    return TRUE;

  // Must be a standard dialog class (#32770).
  char class_name[64];
  GetClassNameA(hwnd, class_name, sizeof(class_name));
  if (strcmp(class_name, "#32770") != 0)
    return TRUE;

  *reinterpret_cast<HWND*>(lParam) = hwnd;
  return FALSE;  // stop enumeration
}

void WatcherThread(int timeout_ms) {
  const int kIntervalMs = 100;
  int elapsed = 0;

  while (g_watching.load() && elapsed < timeout_ms) {
    HWND dialog = NULL;
    EnumWindows(FindPrintDialog, reinterpret_cast<LPARAM>(&dialog));

    if (dialog) {
      // IDOK = Print, IDCANCEL = Cancel.  PostMessage is safe from any
      // thread — the message is processed by the nested message loop
      // inside PrintDlgEx().
      WPARAM cmd = g_should_print.load() ? IDOK : IDCANCEL;
      PostMessage(dialog, WM_COMMAND, cmd, 0);
      g_dismissed.store(true);
      g_watching.store(false);
      return;
    }

    Sleep(kIntervalMs);
    elapsed += kIntervalMs;
  }

  g_watching.store(false);
}

}  // namespace

namespace print_handler {

void StartWatching(bool should_print, int timeout_ms) {
  // Tear down any previous watcher.
  StopWatching();

  g_should_print.store(should_print);
  g_dismissed.store(false);
  g_watching.store(true);

  g_thread = std::thread(WatcherThread, timeout_ms);
}

bool StopWatching() {
  g_watching.store(false);
  if (g_thread.joinable())
    g_thread.join();

  bool result = g_dismissed.load();
  g_dismissed.store(false);
  return result;
}

}  // namespace print_handler
