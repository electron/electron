// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WINDOW_LIST_OBSERVER_H_
#define ELECTRON_SHELL_BROWSER_WINDOW_LIST_OBSERVER_H_

#include "base/observer_list_types.h"

namespace electron {

class NativeWindow;

class WindowListObserver : public base::CheckedObserver {
 public:
  // Called when a window close is cancelled by beforeunload handler.
  virtual void OnWindowCloseCancelled(NativeWindow* window) {}

  // Called immediately after all windows are closed.
  virtual void OnWindowAllClosed() {}

 protected:
  ~WindowListObserver() override = default;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WINDOW_LIST_OBSERVER_H_
