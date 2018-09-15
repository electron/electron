// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#include <memory>
#include <string>

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/browser_observer.h"
#include "atom/browser/login_handler.h"
#include "atom/browser/native_window.h"
#include "atom/browser/window_list.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brightray/browser/brightray_paths.h"
#include "brightray/common/application_info.h"

namespace atom {

Browser::LoginItemSettings::LoginItemSettings() = default;
Browser::LoginItemSettings::~LoginItemSettings() = default;
Browser::LoginItemSettings::LoginItemSettings(const LoginItemSettings& other) =
    default;

Browser::Browser() {
  WindowList::AddObserver(this);
}

Browser::~Browser() {
  WindowList::RemoveObserver(this);
}

// static
Browser* Browser::Get() {
  return AtomBrowserMainParts::Get()->browser();
}

void Browser::Quit() {
  if (is_quiting_)
    return;

  is_quiting_ = HandleBeforeQuit();
  if (!is_quiting_)
    return;

  if (atom::WindowList::IsEmpty())
    NotifyAndShutdown();
  else
    atom::WindowList::CloseAllWindows();
}

void Browser::Exit(mate::Arguments* args) {
  int code = 0;
  args->GetNext(&code);

  if (!AtomBrowserMainParts::Get()->SetExitCode(code)) {
    // Message loop is not ready, quit directly.
    exit(code);
  } else {
    // Prepare to quit when all windows have been closed.
    is_quiting_ = true;

    // Remember this caller so that we don't emit unrelated events.
    is_exiting_ = true;

    // Must destroy windows before quitting, otherwise bad things can happen.
    if (atom::WindowList::IsEmpty()) {
      Shutdown();
    } else {
      // Unlike Quit(), we do not ask to close window, but destroy the window
      // without asking.
      atom::WindowList::DestroyAllWindows();
    }
  }
}

void Browser::Shutdown() {
  if (is_shutdown_)
    return;

  is_shutdown_ = true;
  is_quiting_ = true;

  for (BrowserObserver& observer : observers_)
    observer.OnQuit();

  if (base::ThreadTaskRunnerHandle::IsSet()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::MessageLoop::QuitWhenIdleClosure());
  } else {
    // There is no message loop available so we are in early stage.
    exit(0);
  }
}

std::string Browser::GetVersion() const {
  std::string ret = brightray::GetOverriddenApplicationVersion();
  if (ret.empty())
    ret = GetExecutableFileVersion();
  return ret;
}

void Browser::SetVersion(const std::string& version) {
  brightray::OverrideApplicationVersion(version);
}

std::string Browser::GetName() const {
  std::string ret = brightray::GetOverriddenApplicationName();
  if (ret.empty())
    ret = GetExecutableFileProductName();
  return ret;
}

void Browser::SetName(const std::string& name) {
  brightray::OverrideApplicationName(name);
}

int Browser::GetBadgeCount() {
  return badge_count_;
}

bool Browser::OpenFile(const std::string& file_path) {
  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnOpenFile(&prevent_default, file_path);

  return prevent_default;
}

void Browser::OpenURL(const std::string& url) {
  for (BrowserObserver& observer : observers_)
    observer.OnOpenURL(url);
}

void Browser::Activate(bool has_visible_windows) {
  for (BrowserObserver& observer : observers_)
    observer.OnActivate(has_visible_windows);
}

void Browser::WillFinishLaunching() {
  for (BrowserObserver& observer : observers_)
    observer.OnWillFinishLaunching();
}

void Browser::DidFinishLaunching(const base::DictionaryValue& launch_info) {
  // Make sure the userData directory is created.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::FilePath user_data;
  if (base::PathService::Get(brightray::DIR_USER_DATA, &user_data))
    base::CreateDirectoryAndGetError(user_data, nullptr);

  is_ready_ = true;
  if (ready_promise_) {
    ready_promise_->Resolve();
  }
  for (BrowserObserver& observer : observers_)
    observer.OnFinishLaunching(launch_info);
}

util::Promise* Browser::WhenReady(v8::Isolate* isolate) {
  if (!ready_promise_) {
    ready_promise_ = new util::Promise(isolate);
    if (is_ready()) {
      ready_promise_->Resolve();
    }
  }
  return ready_promise_;
}

void Browser::OnAccessibilitySupportChanged() {
  for (BrowserObserver& observer : observers_)
    observer.OnAccessibilitySupportChanged();
}

void Browser::RequestLogin(
    scoped_refptr<LoginHandler> login_handler,
    std::unique_ptr<base::DictionaryValue> request_details) {
  for (BrowserObserver& observer : observers_)
    observer.OnLogin(login_handler, *(request_details.get()));
}

void Browser::PreMainMessageLoopRun() {
  for (BrowserObserver& observer : observers_) {
    observer.OnPreMainMessageLoopRun();
  }
}

void Browser::NotifyAndShutdown() {
  if (is_shutdown_)
    return;

  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnWillQuit(&prevent_default);

  if (prevent_default) {
    is_quiting_ = false;
    return;
  }

  Shutdown();
}

bool Browser::HandleBeforeQuit() {
  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnBeforeQuit(&prevent_default);

  return !prevent_default;
}

void Browser::OnWindowCloseCancelled(NativeWindow* window) {
  if (is_quiting_)
    // Once a beforeunload handler has prevented the closing, we think the quit
    // is cancelled too.
    is_quiting_ = false;
}

void Browser::OnWindowAllClosed() {
  if (is_exiting_) {
    Shutdown();
  } else if (is_quiting_) {
    NotifyAndShutdown();
  } else {
    for (BrowserObserver& observer : observers_)
      observer.OnWindowAllClosed();
  }
}

#if defined(OS_MACOSX)
void Browser::NewWindowForTab() {
  for (BrowserObserver& observer : observers_)
    observer.OnNewWindowForTab();
}
#endif

}  // namespace atom
