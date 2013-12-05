// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/browser.h"

#include "browser/atom_browser_main_parts.h"
#include "browser/window_list.h"

namespace atom {

Browser::Browser()
    : is_quiting_(false) {
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
  is_quiting_ = true;

  atom::WindowList* window_list = atom::WindowList::GetInstance();
  if (window_list->size() == 0)
    NotifyAndTerminate();

  window_list->CloseAllWindows();
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
  return name_override_.empty() ? "Atom-Shell" : name_override_;
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

void Browser::WillFinishLaunching() {
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnWillFinishLaunching());
}

void Browser::DidFinishLaunching() {
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnFinishLaunching());
}

void Browser::NotifyAndTerminate() {
  bool prevent_default = false;
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnWillQuit(&prevent_default));

  if (prevent_default) {
    is_quiting_ = false;
    return;
  }

  Terminate();
}

void Browser::OnWindowCloseCancelled(NativeWindow* window) {
  if (is_quiting_) {
    // Once a beforeunload handler has prevented the closing, we think the quit
    // is cancelled too.
    is_quiting_ = false;

    CancelQuit();
  }
}

void Browser::OnWindowAllClosed() {
  if (is_quiting_)
    NotifyAndTerminate();
  else
    FOR_EACH_OBSERVER(BrowserObserver, observers_, OnWindowAllClosed());
}

}  // namespace atom
