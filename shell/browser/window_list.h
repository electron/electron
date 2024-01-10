// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WINDOW_LIST_H_
#define ELECTRON_SHELL_BROWSER_WINDOW_LIST_H_

#include <vector>

#include "base/observer_list.h"

namespace electron {

class NativeWindow;
class WindowListObserver;

class WindowList {
 public:
  // disable copy
  WindowList(const WindowList&) = delete;
  WindowList& operator=(const WindowList&) = delete;

  typedef std::vector<NativeWindow*> WindowVector;

  static WindowVector GetWindows();
  static bool IsEmpty();

  // Adds or removes |window| from the list it is associated with.
  static void AddWindow(NativeWindow* window);
  static void RemoveWindow(NativeWindow* window);

  // Called by window when a close is cancelled by beforeunload handler.
  static void WindowCloseCancelled(NativeWindow* window);

  // Adds and removes |observer| from the observer list.
  static void AddObserver(WindowListObserver* observer);
  static void RemoveObserver(WindowListObserver* observer);

  // Closes all windows.
  static void CloseAllWindows();

  // Destroy all windows.
  static void DestroyAllWindows();

 private:
  static WindowList* GetInstance();

  WindowList();
  ~WindowList();

  // A list of observers which will be notified of every window addition and
  // removal across all WindowLists.
  [[nodiscard]] static base::ObserverList<WindowListObserver>& GetObservers();

  // A vector of the windows in this list, in the order they were added.
  WindowVector windows_;

  static WindowList* instance_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WINDOW_LIST_H_
