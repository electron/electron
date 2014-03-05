// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROSER_BROWSER_OBSERVER_H_
#define ATOM_BROSER_BROWSER_OBSERVER_H_

#include <string>

namespace atom {

class BrowserObserver {
 public:
  // The browser has closed all windows and will quit.
  virtual void OnWillQuit(bool* prevent_default) {}

  // The browser has closed all windows. If the browser is quiting, then this
  // method will not be called, instead it will call OnWillQuit.
  virtual void OnWindowAllClosed() {}

  // The browser has opened a file by double clicking in Finder or dragging the
  // file to the Dock icon. (OS X only)
  virtual void OnOpenFile(bool* prevent_default,
                          const std::string& file_path) {}

  // Browser is used to open a url.
  virtual void OnOpenURL(const std::string& url) {}

  // The browser is activated with no open windows (usually by clicking on the
  // dock icon).
  virtual void OnActivateWithNoOpenWindows() {}

  // The browser has finished loading.
  virtual void OnWillFinishLaunching() {}
  virtual void OnFinishLaunching() {}

 protected:
  virtual ~BrowserObserver() {}
};

}  // namespace atom

#endif  // ATOM_BROSER_BROWSER_OBSERVER_H_
