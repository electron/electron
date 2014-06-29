// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_X_ACTIVE_WINDOW_WATCHER_X_H_
#define UI_BASE_X_ACTIVE_WINDOW_WATCHER_X_H_

#include "base/basictypes.h"
#include "base/observer_list.h"
#include "ui/base/ui_base_export.h"
#include "ui/base/x/x11_util.h"

template <typename T> struct DefaultSingletonTraits;

namespace ui {

class ActiveWindowWatcherXObserver;

namespace internal {
class RootWindowPropertyWatcherX;
}

// This is a helper class that is used to keep track of which window the X
// window manager thinks is active. Add an Observer to listen for changes to
// the active window.
class UI_BASE_EXPORT ActiveWindowWatcherX {
 public:
  static ActiveWindowWatcherX* GetInstance();
  static void AddObserver(ActiveWindowWatcherXObserver* observer);
  static void RemoveObserver(ActiveWindowWatcherXObserver* observer);

  // Checks if the WM supports the active window property. Note that the return
  // value can change, especially during system startup.
  static bool WMSupportsActivation();

 private:
  friend struct DefaultSingletonTraits<ActiveWindowWatcherX>;
  friend class ui::internal::RootWindowPropertyWatcherX;

  ActiveWindowWatcherX();
  ~ActiveWindowWatcherX();

  // Gets the atom for the default display for the property this class is
  // watching for.
  static Atom GetPropertyAtom();

  // Notify observers that the active window has changed.
  static void Notify();

  // Instance method that implements Notify().
  void NotifyActiveWindowChanged();

  ObserverList<ActiveWindowWatcherXObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(ActiveWindowWatcherX);
};

}  // namespace ui

#endif  // UI_BASE_X_ACTIVE_WINDOW_WATCHER_X_H_
