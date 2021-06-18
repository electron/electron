// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/ui/views/electron_views_delegate.h"

#include <dwmapi.h>
#include <shellapi.h>

#include <utility>

#include "base/bind.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"

namespace {

bool MonitorHasAutohideTaskbarForEdge(UINT edge, HMONITOR monitor) {
  APPBARDATA taskbar_data = {sizeof(APPBARDATA), NULL, 0, edge};
  taskbar_data.hWnd = ::GetForegroundWindow();

  // MSDN documents an ABM_GETAUTOHIDEBAREX, which supposedly takes a monitor
  // rect and returns autohide bars on that monitor.  This sounds like a good
  // idea for multi-monitor systems.  Unfortunately, it appears to not work at
  // least some of the time (erroneously returning NULL) and there's almost no
  // online documentation or other sample code using it that suggests ways to
  // address this problem. We do the following:-
  // 1. Use the ABM_GETAUTOHIDEBAR message. If it works, i.e. returns a valid
  //    window we are done.
  // 2. If the ABM_GETAUTOHIDEBAR message does not work we query the auto hide
  //    state of the taskbar and then retrieve its position. That call returns
  //    the edge on which the taskbar is present. If it matches the edge we
  //    are looking for, we are done.
  // NOTE: This call spins a nested run loop.
  HWND taskbar = reinterpret_cast<HWND>(
      SHAppBarMessage(ABM_GETAUTOHIDEBAR, &taskbar_data));
  if (!::IsWindow(taskbar)) {
    APPBARDATA taskbar_data = {sizeof(APPBARDATA), 0, 0, 0};
    unsigned int taskbar_state = SHAppBarMessage(ABM_GETSTATE, &taskbar_data);
    if (!(taskbar_state & ABS_AUTOHIDE))
      return false;

    taskbar_data.hWnd = ::FindWindow(L"Shell_TrayWnd", NULL);
    if (!::IsWindow(taskbar_data.hWnd))
      return false;

    SHAppBarMessage(ABM_GETTASKBARPOS, &taskbar_data);
    if (taskbar_data.uEdge == edge)
      taskbar = taskbar_data.hWnd;
  }

  // There is a potential race condition here:
  // 1. A maximized chrome window is fullscreened.
  // 2. It is switched back to maximized.
  // 3. In the process the window gets a WM_NCCACLSIZE message which calls us to
  //    get the autohide state.
  // 4. The worker thread is invoked. It calls the API to get the autohide
  //    state. On Windows versions  earlier than Windows 7, taskbars could
  //    easily be always on top or not.
  //    This meant that we only want to look for taskbars which have the topmost
  //    bit set.  However this causes problems in cases where the window on the
  //    main thread is still in the process of switching away from fullscreen.
  //    In this case the taskbar might not yet have the topmost bit set.
  // 5. The main thread resumes and does not leave space for the taskbar and
  //    hence it does not pop when hovered.
  //
  // To address point 4 above, it is best to not check for the WS_EX_TOPMOST
  // window style on the taskbar, as starting from Windows 7, the topmost
  // style is always set. We don't support XP and Vista anymore.
  if (::IsWindow(taskbar)) {
    if (MonitorFromWindow(taskbar, MONITOR_DEFAULTTONEAREST) == monitor)
      return true;
    // In some cases like when the autohide taskbar is on the left of the
    // secondary monitor, the MonitorFromWindow call above fails to return the
    // correct monitor the taskbar is on. We fallback to MonitorFromPoint for
    // the cursor position in that case, which seems to work well.
    POINT cursor_pos = {0};
    GetCursorPos(&cursor_pos);
    if (MonitorFromPoint(cursor_pos, MONITOR_DEFAULTTONEAREST) == monitor)
      return true;
  }
  return false;
}

int GetAppbarAutohideEdgesOnWorkerThread(HMONITOR monitor) {
  DCHECK(monitor);

  int edges = 0;
  if (MonitorHasAutohideTaskbarForEdge(ABE_LEFT, monitor))
    edges |= views::ViewsDelegate::EDGE_LEFT;
  if (MonitorHasAutohideTaskbarForEdge(ABE_TOP, monitor))
    edges |= views::ViewsDelegate::EDGE_TOP;
  if (MonitorHasAutohideTaskbarForEdge(ABE_RIGHT, monitor))
    edges |= views::ViewsDelegate::EDGE_RIGHT;
  if (MonitorHasAutohideTaskbarForEdge(ABE_BOTTOM, monitor))
    edges |= views::ViewsDelegate::EDGE_BOTTOM;
  return edges;
}

}  // namespace

namespace electron {

HICON ViewsDelegate::GetDefaultWindowIcon() const {
  // Use current exe's icon as default window icon.
  return LoadIcon(GetModuleHandle(NULL),
                  MAKEINTRESOURCE(1 /* IDR_MAINFRAME */));
}

HICON ViewsDelegate::GetSmallWindowIcon() const {
  return GetDefaultWindowIcon();
}

bool ViewsDelegate::IsWindowInMetro(gfx::NativeWindow window) const {
  return false;
}

int ViewsDelegate::GetAppbarAutohideEdges(HMONITOR monitor,
                                          base::OnceClosure callback) {
  // Initialize the map with EDGE_BOTTOM. This is important, as if we return an
  // initial value of 0 (no auto-hide edges) then we'll go fullscreen and
  // windows will automatically remove WS_EX_TOPMOST from the appbar resulting
  // in us thinking there is no auto-hide edges. By returning at least one edge
  // we don't initially go fullscreen until we figure out the real auto-hide
  // edges.
  if (!appbar_autohide_edge_map_.count(monitor))
    appbar_autohide_edge_map_[monitor] = EDGE_BOTTOM;

  // We use the SHAppBarMessage API to get the taskbar autohide state. This API
  // spins a modal loop which could cause callers to be reentered. To avoid
  // that we retrieve the taskbar state in a worker thread.
  if (monitor && !in_autohide_edges_callback_) {
    // TODO(robliao): Annotate this task with .WithCOM() once supported.
    // https://crbug.com/662122
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
        base::BindOnce(&GetAppbarAutohideEdgesOnWorkerThread, monitor),
        base::BindOnce(&ViewsDelegate::OnGotAppbarAutohideEdges,
                       weak_factory_.GetWeakPtr(), std::move(callback), monitor,
                       appbar_autohide_edge_map_[monitor]));
  }
  return appbar_autohide_edge_map_[monitor];
}

void ViewsDelegate::OnGotAppbarAutohideEdges(base::OnceClosure callback,
                                             HMONITOR monitor,
                                             int returned_edges,
                                             int edges) {
  appbar_autohide_edge_map_[monitor] = edges;
  if (returned_edges == edges)
    return;

  base::AutoReset<bool> in_callback_setter(&in_autohide_edges_callback_, true);
  std::move(callback).Run();
}

}  // namespace electron
