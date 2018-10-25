// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_BROWSER_OBSERVER_H_
#define ATOM_BROWSER_BROWSER_OBSERVER_H_

#include <string>

#include "atom/browser/login_handler.h"
#include "base/memory/scoped_refptr.h"
#include "base/observer_list_types.h"
#include "build/build_config.h"

namespace base {
class DictionaryValue;
}

namespace atom {

class BrowserObserver : public base::CheckedObserver {
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
  virtual void OnOpenFile(bool* prevent_default, const std::string& file_path) {
  }

  // Browser is used to open a url.
  virtual void OnOpenURL(const std::string& url) {}

  // The browser is activated with visible/invisible windows (usually by
  // clicking on the dock icon).
  virtual void OnActivate(bool has_visible_windows) {}

  // The browser has finished loading.
  virtual void OnWillFinishLaunching() {}
  virtual void OnFinishLaunching(const base::DictionaryValue& launch_info) {}

  // The browser requests HTTP login.
  virtual void OnLogin(scoped_refptr<LoginHandler> login_handler,
                       const base::DictionaryValue& request_details) {}

  // The browser's accessibility suppport has changed.
  virtual void OnAccessibilitySupportChanged() {}

  // The app message loop is ready
  virtual void OnPreMainMessageLoopRun() {}

#if defined(OS_MACOSX)
  // The browser wants to report that an user activity will resume. (macOS only)
  virtual void OnWillContinueUserActivity(bool* prevent_default,
                                          const std::string& type) {}
  // The browser wants to report an user activity resuming error. (macOS only)
  virtual void OnDidFailToContinueUserActivity(const std::string& type,
                                               const std::string& error) {}
  // The browser wants to resume a user activity via handoff. (macOS only)
  virtual void OnContinueUserActivity(bool* prevent_default,
                                      const std::string& type,
                                      const base::DictionaryValue& user_info) {}
  // The browser wants to notify that an user activity was resumed. (macOS only)
  virtual void OnUserActivityWasContinued(
      const std::string& type,
      const base::DictionaryValue& user_info) {}
  // The browser wants to update an user activity payload. (macOS only)
  virtual void OnUpdateUserActivityState(
      bool* prevent_default,
      const std::string& type,
      const base::DictionaryValue& user_info) {}
  // User clicked the native macOS new tab button. (macOS only)
  virtual void OnNewWindowForTab() {}
#endif

 protected:
  ~BrowserObserver() override {}
};

}  // namespace atom

#endif  // ATOM_BROWSER_BROWSER_OBSERVER_H_
