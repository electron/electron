// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_BROWSER_OBSERVER_H_
#define ATOM_BROWSER_BROWSER_OBSERVER_H_

#include <string>

#include "build/build_config.h"

namespace base {
class DictionaryValue;
}

namespace atom {

class LoginHandler;

class BrowserObserver {
 public:
  // The browser is about to close all windows.
  virtual void OnBeforeQuit(bool* prevent_default) {}

  // The browser has closed all windows and will quit.
  virtual void OnWillQuit(bool* prevent_default) {}

  // The browser has closed all windows. If the browser is quiting, then this
  // method will not be called, instead it will call OnWillQuit.
  virtual void OnWindowAllClosed() {}

  // The browser is quitting.
  virtual void OnQuit() {}

  // The browser has opened a file by double clicking in Finder or dragging the
  // file to the Dock icon. (macOS only)
  virtual void OnOpenFile(bool* prevent_default,
                          const std::string& file_path) {}

  // Browser is used to open a url.
  virtual void OnOpenURL(const std::string& url) {}

  // The browser is activated with visible/invisible windows (usually by
  // clicking on the dock icon).
  virtual void OnActivate(bool has_visible_windows) {}

  // The browser has finished loading.
  virtual void OnWillFinishLaunching() {}
  virtual void OnFinishLaunching() {}

  // The browser requests HTTP login.
  virtual void OnLogin(LoginHandler* login_handler,
                       const base::DictionaryValue& request_details) {}

  // The browser's accessibility suppport has changed.
  virtual void OnAccessibilitySupportChanged() {}

#if defined(OS_MACOSX)
  // The browser wants to resume a user activity via handoff. (macOS only)
  virtual void OnContinueUserActivity(
      bool* prevent_default,
      const std::string& type,
      const base::DictionaryValue& user_info) {}
#endif

 protected:
  virtual ~BrowserObserver() {}
};

}  // namespace atom

#endif  // ATOM_BROWSER_BROWSER_OBSERVER_H_
