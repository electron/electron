// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#include <utility>

#include "base/logging.h"
#include "base/task/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/win_util.h"
#include "base/win/wrapped_window_proc.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/lib/shutdown_blocker_win.h"
#include "ui/gfx/win/hwnd_util.h"

namespace electron {

namespace {
const wchar_t kShutdownBlockerWinWindowClass[] = L"Electron_ShutdownBlockerWin";
}

ShutdownBlockerWin::ShutdownBlockerWin(bool dedicated_message_loop)
    : dedicated_message_loop_(dedicated_message_loop),
      instance_(NULL),
      window_(NULL) {}

ShutdownBlockerWin::~ShutdownBlockerWin() = default;

void ShutdownBlockerWin::SetShutdownHandler(base::Callback<bool()> handler) {
  LOG(INFO) << "setting shutdown handler on "
            << (dedicated_message_loop_ ? "renderer" : "browser") << " process";
  should_shutdown_ = std::move(handler);
  BlockShutdown();
}

void ShutdownBlockerWin::BlockShutdown() {
  if (blocking_) {
    return;
  }
  blocking_ = true;
  if (dedicated_message_loop_) {
    LOG(INFO) << "unblocking shutdown on renderer process";
    owner_thread_task_runner_ = base::ThreadTaskRunnerHandle::Get();
    owner_thread_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&ShutdownBlockerWin::MessageLoop,
                                  base::Unretained(this)));
  } else {
    LOG(INFO) << "blocking shutdown on browser process";
    CreateHiddenWindow();
  }
}

void ShutdownBlockerWin::MessageLoop() {
  CreateHiddenWindow();
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  DestroyHiddenWindow();
}

void ShutdownBlockerWin::CreateHiddenWindow() {
  ATOM atom = RegisterWindowClass();
  window_ = CreateWindow(MAKEINTATOM(atom), 0, WS_POPUP, 0, 0, 0, 0, 0, 0,
                         instance_, 0);
  gfx::CheckWindowCreated(window_, ::GetLastError());
  gfx::SetWindowUserData(window_, this);
}

void ShutdownBlockerWin::DestroyHiddenWindow() {
  DestroyWindow(window_);
  UnregisterClass(kShutdownBlockerWinWindowClass, instance_);
}

ATOM ShutdownBlockerWin::RegisterWindowClass() {
  // Register our window class
  WNDCLASSEX window_class;
  base::win::InitializeWindowClass(
      kShutdownBlockerWinWindowClass,
      &base::win::WrappedWindowProc<ShutdownBlockerWin::WndProc>, 0, 0, 0, NULL,
      NULL, NULL, NULL, NULL, &window_class);
  ATOM atom = RegisterClassEx(&window_class);
  instance_ = window_class.hInstance;
  return atom;
}

bool ShutdownBlockerWin::OnQueryEndSession() {
  if (dedicated_message_loop_) {
    owner_thread_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&ShutdownBlockerWin::OwnerThreadShutdownHandler,
                       base::Unretained(this)));
    return false;
  }
  return !should_shutdown_ || should_shutdown_.Run();
}

void ShutdownBlockerWin::OwnerThreadShutdownHandler() {
  if (should_shutdown_) {
    should_shutdown_.Run();
  }
}

LRESULT CALLBACK ShutdownBlockerWin::WndProc(HWND hwnd,
                                             UINT message,
                                             WPARAM wparam,
                                             LPARAM lparam) {
  switch (message) {
    case WM_QUERYENDSESSION: {
      ShutdownBlockerWin* blocker = reinterpret_cast<ShutdownBlockerWin*>(
          GetWindowLongPtr(hwnd, GWLP_USERDATA));
      LOG(INFO) << "Received WM_QUERYENDSESSION"
                << (!blocker ? ""
                             : (blocker->dedicated_message_loop_
                                    ? " on renderer process"
                                    : " on browser process"));
      if (blocker && !blocker->OnQueryEndSession()) {
        LOG(INFO) << "Shutdown blocked";
        ShutdownBlockReasonCreate(hwnd, L"Ensure a clean shutdown");
        return FALSE;
      }
      return TRUE;
    }
    default:
      return ::DefWindowProc(hwnd, message, wparam, lparam);
  }
}

}  // namespace electron
