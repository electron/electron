// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
//
// The purpose of this class is to block system shutdown on windows so the
// application can exit cleanly. It uses the WM_QUERYENDSESSION message to do
// so, as explained here
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa376890(v=vs.85).aspx
//
// The WM_QUERYENDSESSION message must be handled in every every process managed
// by an application, so we encapsulate the logic in a class that is common to
// both browser and renderer processes.
//
// In other words: it is not enough to handle this message in the browser
// process and emit an event to the `powerMonitor` module (like it is done for
// Linux/OSX), because even if the browser process blocks shutdown, windows will
// still kill the renderers leaving the application in an invalid state.
//
// This class has two modes of operation: It can use the main thread message
// loop (browser process) or it can run a dedicated message loop (renderer
// process). When it uses a dedicated message loop, it must run in a separate
// thread to avoid blocking the blink/webkit thread. In this case, when the
// WM_QUERYENDSESSION message is received, it will always return false (blocking
// shutdown) and will also invoke the user callback in the renderer main thread.
//
// This is done to forward the message to the browser process, since when the
// renderer receives the WM_QUERYENDSESSION message first, the browser process
// will not receive it and must be notified that the system is shutting down via
// IPC. See lib/browser/api/power-monitor.js and
// lib/renderer/win32-queryendsession-setup.js for details of how IPC is handled
// on the JS side.
//
// In all my tests the renderer received the WM_QUERYENDSESSION message first,
// blocking the browser process from receiving it, but I've found no evidence
// that child processes will always receive it first, so this class must also be
// used in the browser process in which case it will deal with shutdown
// by directly emitting the powerMonitor "shutdown" event.
#ifndef SHELL_BROWSER_LIB_SHUTDOWN_BLOCKER_WIN_H_
#define SHELL_BROWSER_LIB_SHUTDOWN_BLOCKER_WIN_H_

#include <windows.h>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"

namespace electron {

class ShutdownBlockerWin {
 public:
  explicit ShutdownBlockerWin(bool dedicated_message_loop);
  ~ShutdownBlockerWin();

  void SetShutdownHandler(base::Callback<bool()> should_shutdown);

 private:
  void BlockShutdown();
  void MessageLoop();
  void CreateHiddenWindow();
  void DestroyHiddenWindow();
  ATOM RegisterWindowClass();
  bool OnQueryEndSession();
  void OwnerThreadShutdownHandler();

  // Static callback invoked when a message comes in to our messaging window.
  static LRESULT CALLBACK WndProc(HWND hwnd,
                                  UINT message,
                                  WPARAM wparam,
                                  LPARAM lparam);

  bool dedicated_message_loop_;
  bool blocking_ = false;
  scoped_refptr<base::SingleThreadTaskRunner> owner_thread_task_runner_;
  base::Callback<bool()> should_shutdown_;
  HINSTANCE instance_;
  HWND window_;
  DISALLOW_COPY_AND_ASSIGN(ShutdownBlockerWin);
};

}  // namespace electron

#endif  // SHELL_BROWSER_LIB_SHUTDOWN_BLOCKER_WIN_H_
