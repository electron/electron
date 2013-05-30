// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_BROWSER_H_
#define ATOM_BROWSER_BROWSER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/observer_list.h"
#include "browser/browser_observer.h"
#include "browser/window_list_observer.h"

namespace atom {

// This class is used for control application-wide operations.
class Browser : public WindowListObserver {
 public:
  Browser();
  ~Browser();

  static Browser* Get();

  // Try to close all windows and quit the application.
  void Quit();

  // Quit the application immediately without cleanup work.
  void Terminate();

  // Tell the application to open a file.
  bool OpenFile(const std::string& file_path);

  void AddObserver(BrowserObserver* obs) {
    observers_.AddObserver(obs);
  }

  void RemoveObserver(BrowserObserver* obs) {
    observers_.RemoveObserver(obs);
  }

 protected:
  void NotifyAndTerminate();

  bool is_quiting_;

 private:
  // WindowListObserver implementations:
  virtual void OnWindowCloseCancelled(NativeWindow* window) OVERRIDE;
  virtual void OnWindowAllClosed() OVERRIDE;

  // Observers of the browser.
  ObserverList<BrowserObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(Browser);
};

}  // namespace atom

#endif  // ATOM_BROWSER_BROWSER_H_
