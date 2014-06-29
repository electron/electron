// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/x/work_area_watcher_x.h"

#include "base/memory/singleton.h"
#include "ui/base/work_area_watcher_observer.h"
#include "ui/base/x/root_window_property_watcher_x.h"
#include "ui/base/x/x11_util.h"

namespace ui {

static const char* const kNetWorkArea = "_NET_WORKAREA";

// static
WorkAreaWatcherX* WorkAreaWatcherX::GetInstance() {
  return Singleton<WorkAreaWatcherX>::get();
}

// static
void WorkAreaWatcherX::AddObserver(WorkAreaWatcherObserver* observer) {
  // Ensure that RootWindowPropertyWatcherX exists.
  internal::RootWindowPropertyWatcherX::GetInstance();
  GetInstance()->observers_.AddObserver(observer);
}

// static
void WorkAreaWatcherX::RemoveObserver(WorkAreaWatcherObserver* observer) {
  GetInstance()->observers_.RemoveObserver(observer);
}

// static
void WorkAreaWatcherX::Notify() {
  GetInstance()->NotifyWorkAreaChanged();
}

// static
Atom WorkAreaWatcherX::GetPropertyAtom() {
  return GetAtom(kNetWorkArea);
}

WorkAreaWatcherX::WorkAreaWatcherX() {
}

WorkAreaWatcherX::~WorkAreaWatcherX() {
}

void WorkAreaWatcherX::NotifyWorkAreaChanged() {
  FOR_EACH_OBSERVER(WorkAreaWatcherObserver, observers_, WorkAreaChanged());
}

}  // namespace ui
