// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/notify_icon_host.h"

#include <commctrl.h>

#include "atom/browser/ui/win/notify_icon.h"
#include "base/bind.h"
#include "base/stl_util.h"
#include "base/threading/non_thread_safe.h"
#include "base/threading/thread.h"
#include "base/win/wrapped_window_proc.h"
#include "chrome/browser/ui/views/status_icons/status_tray_state_changer_win.h"
#include "ui/gfx/screen.h"
#include "ui/gfx/win/hwnd_util.h"

namespace atom {

namespace {

const UINT kNotifyIconMessage = WM_APP + 1;

// |kBaseIconId| is 2 to avoid conflicts with plugins that hard-code id 1.
const UINT kBaseIconId = 2;

const wchar_t kNotifyIconHostWindowClass[] = L"AtomShell_NotifyIconHostWindow";

}  // namespace

// Default implementation for NotifyIconHostStateChangerProxy that communicates
// to Exporer.exe via COM.  It spawns a background thread with a fresh COM
// apartment and requests that the visibility be increased unless the user
// has explicitly set the icon to be hidden.
class NotifyIconHostStateChangerProxyImpl
    : public NotifyIconHostStateChangerProxy,
      public base::NonThreadSafe {
 public:
  NotifyIconHostStateChangerProxyImpl()
      : pending_requests_(0),
        worker_thread_("NotifyIconCOMWorkerThread"),
        weak_factory_(this) {
    worker_thread_.init_com_with_mta(false);
  }

  virtual void EnqueueChange(UINT icon_id, HWND window) OVERRIDE {
    DCHECK(CalledOnValidThread());
    if (pending_requests_ == 0)
      worker_thread_.Start();

    ++pending_requests_;
    worker_thread_.message_loop_proxy()->PostTaskAndReply(
        FROM_HERE,
        base::Bind(
            &NotifyIconHostStateChangerProxyImpl::EnqueueChangeOnWorkerThread,
            icon_id,
            window),
        base::Bind(&NotifyIconHostStateChangerProxyImpl::ChangeDone,
                   weak_factory_.GetWeakPtr()));
  }

 private:
  // Must be called only on |worker_thread_|, to ensure the correct COM
  // apartment.
  static void EnqueueChangeOnWorkerThread(UINT icon_id, HWND window) {
    // It appears that IUnknowns are coincidentally compatible with
    // scoped_refptr.  Normally I wouldn't depend on that but it seems that
    // base::win::IUnknownImpl itself depends on that coincidence so it's
    // already being assumed elsewhere.
    scoped_refptr<StatusTrayStateChangerWin> status_tray_state_changer(
        new StatusTrayStateChangerWin(icon_id, window));
    status_tray_state_changer->EnsureTrayIconVisible();
  }

  // Called on UI thread.
  void ChangeDone() {
    DCHECK(CalledOnValidThread());
    DCHECK_GT(pending_requests_, 0);

    if (--pending_requests_ == 0)
      worker_thread_.Stop();
  }

 private:
  int pending_requests_;
  base::Thread worker_thread_;
  base::WeakPtrFactory<NotifyIconHostStateChangerProxyImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NotifyIconHostStateChangerProxyImpl);
};


NotifyIconHost::NotifyIconHost()
    : next_icon_id_(1),
      atom_(0),
      instance_(NULL),
      window_(NULL) {
  // Register our window class
  WNDCLASSEX window_class;
  base::win::InitializeWindowClass(
      kNotifyIconHostWindowClass,
      &base::win::WrappedWindowProc<NotifyIconHost::WndProcStatic>,
      0, 0, 0, NULL, NULL, NULL, NULL, NULL,
      &window_class);
  instance_ = window_class.hInstance;
  atom_ = RegisterClassEx(&window_class);
  CHECK(atom_);

  // If the taskbar is re-created after we start up, we have to rebuild all of
  // our icons.
  taskbar_created_message_ = RegisterWindowMessage(TEXT("TaskbarCreated"));

  // Create an offscreen window for handling messages for the status icons. We
  // create a hidden WS_POPUP window instead of an HWND_MESSAGE window, because
  // only top-level windows such as popups can receive broadcast messages like
  // "TaskbarCreated".
  window_ = CreateWindow(MAKEINTATOM(atom_),
                         0, WS_POPUP, 0, 0, 0, 0, 0, 0, instance_, 0);
  gfx::CheckWindowCreated(window_);
  gfx::SetWindowUserData(window_, this);
}

NotifyIconHost::~NotifyIconHost() {
  if (window_)
    DestroyWindow(window_);

  if (atom_)
    UnregisterClass(MAKEINTATOM(atom_), instance_);

  NotifyIcons copied_container(notify_icons_);
  STLDeleteContainerPointers(copied_container.begin(), copied_container.end());
}

NotifyIcon* NotifyIconHost::CreateNotifyIcon() {
  NotifyIcon* notify_icon =
      new NotifyIcon(this, NextIconId(), window_, kNotifyIconMessage);
  notify_icons_.push_back(notify_icon);
  return notify_icon;
}

void NotifyIconHost::Remove(NotifyIcon* icon) {
  NotifyIcons::iterator i(
      std::find(notify_icons_.begin(), notify_icons_.end(), icon));

  if (i == notify_icons_.end()) {
    NOTREACHED();
    return;
  }

  notify_icons_.erase(i);
}

void NotifyIconHost::UpdateIconVisibilityInBackground(
    NotifyIcon* notify_icon) {
  if (!state_changer_proxy_.get())
    state_changer_proxy_.reset(new NotifyIconHostStateChangerProxyImpl);

  state_changer_proxy_->EnqueueChange(notify_icon->icon_id(),
                                      notify_icon->window());
}

LRESULT CALLBACK NotifyIconHost::WndProcStatic(HWND hwnd,
                                              UINT message,
                                              WPARAM wparam,
                                              LPARAM lparam) {
  NotifyIconHost* msg_wnd = reinterpret_cast<NotifyIconHost*>(
      GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg_wnd)
    return msg_wnd->WndProc(hwnd, message, wparam, lparam);
  else
    return ::DefWindowProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK NotifyIconHost::WndProc(HWND hwnd,
                                        UINT message,
                                        WPARAM wparam,
                                        LPARAM lparam) {
  if (message == taskbar_created_message_) {
    // We need to reset all of our icons because the taskbar went away.
    for (NotifyIcons::const_iterator i(notify_icons_.begin());
         i != notify_icons_.end(); ++i) {
      NotifyIcon* win_icon = static_cast<NotifyIcon*>(*i);
      win_icon->ResetIcon();
    }
    return TRUE;
  } else if (message == kNotifyIconMessage) {
    NotifyIcon* win_icon = NULL;

    // Find the selected status icon.
    for (NotifyIcons::const_iterator i(notify_icons_.begin());
         i != notify_icons_.end(); ++i) {
      NotifyIcon* current_win_icon = static_cast<NotifyIcon*>(*i);
      if (current_win_icon->icon_id() == wparam) {
        win_icon = current_win_icon;
        break;
      }
    }

    // It is possible for this procedure to be called with an obsolete icon
    // id.  In that case we should just return early before handling any
    // actions.
    if (!win_icon)
      return TRUE;

    switch (lparam) {
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_CONTEXTMENU:
        // Walk our icons, find which one was clicked on, and invoke its
        // HandleClickEvent() method.
        gfx::Point cursor_pos(
            gfx::Screen::GetNativeScreen()->GetCursorScreenPoint());
        win_icon->HandleClickEvent(cursor_pos, lparam == WM_LBUTTONDOWN);
        return TRUE;
    }
  }
  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

UINT NotifyIconHost::NextIconId() {
  UINT icon_id = next_icon_id_++;
  return kBaseIconId + icon_id;
}

}  // namespace atom
