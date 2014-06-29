// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_X_ROOT_WINDOW_PROPERTY_WATCHER_X_H_
#define UI_BASE_X_ROOT_WINDOW_PROPERTY_WATCHER_X_H_

#include <gdk/gdk.h>

#include "base/basictypes.h"
#include "ui/base/gtk/gtk_signal.h"

template <typename T> struct DefaultSingletonTraits;

namespace ui {
namespace internal {

// This class keeps track of changes to properties on the root window. This is
// not to be used directly. Implement a watcher for the specific property you're
// interested in.
class RootWindowPropertyWatcherX {
 public:
  static RootWindowPropertyWatcherX* GetInstance();

 private:
  friend struct DefaultSingletonTraits<RootWindowPropertyWatcherX>;

  RootWindowPropertyWatcherX();
  ~RootWindowPropertyWatcherX();

  // Callback for PropertyChange XEvents.
  CHROMEG_CALLBACK_1(RootWindowPropertyWatcherX, GdkFilterReturn,
                     OnWindowXEvent, GdkXEvent*, GdkEvent*);

  DISALLOW_COPY_AND_ASSIGN(RootWindowPropertyWatcherX);
};

}  // namespace internal
}  // namespace ui

#endif  // UI_BASE_X_ROOT_WINDOW_PROPERTY_WATCHER_X_H_
