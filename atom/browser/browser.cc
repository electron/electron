// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#include <string>

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/window_list.h"
#include "base/message_loop/message_loop.h"

namespace atom {

Browser::Browser()
    : is_quiting_(false),
      is_ready_(false) {
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
  is_quiting_ = HandleBeforeQuit();
  if (!is_quiting_)
    return;

  atom::WindowList* window_list = atom::WindowList::GetInstance();
  if (window_list->size() == 0)
    NotifyAndShutdown();

  window_list->CloseAllWindows();
}

void Browser::Shutdown() {
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnQuit());

  is_quiting_ = true;
  base::MessageLoop::current()->Quit();
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

#if defined(OS_WIN)
  SetAppUserModelID(name);
#endif
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

void Browser::ActivateWithNoOpenWindows() {
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnActivateWithNoOpenWindows());
}

void Browser::WillFinishLaunching() {
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnWillFinishLaunching());
}

void Browser::DidFinishLaunching() {
  is_ready_ = true;
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnFinishLaunching());
}

void Browser::NotifyAndShutdown() {
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
  if (is_quiting_)
    NotifyAndShutdown();
  else
    FOR_EACH_OBSERVER(BrowserObserver, observers_, OnWindowAllClosed());
}

}  // namespace atom
