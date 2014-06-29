// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_X_ACTIVE_WINDOW_WATCHER_X_OBSERVER_H_
#define UI_BASE_X_ACTIVE_WINDOW_WATCHER_X_OBSERVER_H_

#include <gdk/gdk.h>

#include "ui/base/ui_base_export.h"

namespace ui {

class UI_BASE_EXPORT ActiveWindowWatcherXObserver {
 public:
  // |active_window| will be NULL if the active window isn't one of Chrome's.
  virtual void ActiveWindowChanged(GdkWindow* active_window) = 0;

 protected:
  virtual ~ActiveWindowWatcherXObserver() {}
};

}  // namespace ui

#endif  // UI_BASE_X_ACTIVE_WINDOW_WATCHER_X_OBSERVER_H_
