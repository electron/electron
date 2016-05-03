// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#include <string>

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/native_window.h"
#include "atom/browser/window_list.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "brightray/browser/brightray_paths.h"

namespace atom {

Browser::Browser()
    : is_quiting_(false),
      is_exiting_(false),
      is_ready_(false),
      is_shutdown_(false) {
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

  atom::WindowList* window_list = atom::WindowList::GetInstance();
  if (window_list->size() == 0)
    NotifyAndShutdown();

  window_list->CloseAllWindows();
}

void Browser::Exit(int code) {
  if (!AtomBrowserMainParts::Get()->SetExitCode(code)) {
    // Message loop is not ready, quit directly.
    exit(code);
  } else {
    // Prepare to quit when all windows have been closed.
    is_quiting_ = true;

    // Remember this caller so that we don't emit unrelated events.
    is_exiting_ = true;

    // Must destroy windows before quitting, otherwise bad things can happen.
    atom::WindowList* window_list = atom::WindowList::GetInstance();
    if (window_list->size() == 0) {
      Shutdown();
    } else {
      // Unlike Quit(), we do not ask to close window, but destroy the window
      // without asking.
      for (NativeWindow* window : *window_list)
        window->CloseContents(nullptr);  // e.g. Destroy()
    }
  }
}

void Browser::Shutdown() {
  if (is_shutdown_)
    return;

  is_shutdown_ = true;
  is_quiting_ = true;

  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnQuit());

  if (base::MessageLoop::current()) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::MessageLoop::QuitWhenIdleClosure());
  } else {
    // There is no message loop available so we are in early stage.
    exit(0);
  }
}

std::string Browser::GetVersion() const {
  if (version_override_.empty()) {
    std::string version = GetExecutableFileVersion();
    if (!version.empty())
      return version;
  }

  return version_override_;
}

void Browser::SetVersion(const std::string& version) {
  version_override_ = version;
}

std::string Browser::GetName() const {
  if (name_override_.empty()) {
    std::string name = GetExecutableFileProductName();
    if (!name.empty())
      return name;
  }

  return name_override_;
}

void Browser::SetName(const std::string& name) {
  name_override_ = name;
}

bool Browser::OpenFile(const std::string& file_path) {
  bool prevent_default = false;
  FOR_EACH_OBSERVER(BrowserObserver,
                    observers_,
                    OnOpenFile(&prevent_default, file_path));

  return prevent_default;
}

void Browser::OpenURL(const std::string& url) {
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnOpenURL(url));
}

void Browser::Activate(bool has_visible_windows) {
  FOR_EACH_OBSERVER(BrowserObserver,
                    observers_,
                    OnActivate(has_visible_windows));
}

#if defined(OS_MACOSX)
bool Browser::ContinueUserActivity(const std::string& type,
                                   const std::map<std::string,
                                   std::string>& user_info) {
  bool prevent_default = false;
  FOR_EACH_OBSERVER(BrowserObserver,
                    observers_,
                    OnContinueUserActivity(&prevent_default, type, user_info));

  return prevent_default;
}
#endif

void Browser::WillFinishLaunching() {
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnWillFinishLaunching());
}

void Browser::DidFinishLaunching() {
  // Make sure the userData directory is created.
  base::FilePath user_data;
  if (PathService::Get(brightray::DIR_USER_DATA, &user_data))
    base::CreateDirectoryAndGetError(user_data, nullptr);

  is_ready_ = true;
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnFinishLaunching());
}

void Browser::RequestLogin(LoginHandler* login_handler) {
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnLogin(login_handler));
}

void Browser::NotifyAndShutdown() {
  if (is_shutdown_)
    return;

  bool prevent_default = false;
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnWillQuit(&prevent_default));

  if (prevent_default) {
    is_quiting_ = false;
    return;
  }

  Shutdown();
}

bool Browser::HandleBeforeQuit() {
  bool prevent_default = false;
  FOR_EACH_OBSERVER(BrowserObserver,
                    observers_,
                    OnBeforeQuit(&prevent_default));

  return !prevent_default;
}

void Browser::OnWindowCloseCancelled(NativeWindow* window) {
  if (is_quiting_)
    // Once a beforeunload handler has prevented the closing, we think the quit
    // is cancelled too.
    is_quiting_ = false;
}

void Browser::OnWindowAllClosed() {
  if (is_exiting_)
    Shutdown();
  else if (is_quiting_)
    NotifyAndShutdown();
  else
    FOR_EACH_OBSERVER(BrowserObserver, observers_, OnWindowAllClosed());
}

}  // namespace atom
