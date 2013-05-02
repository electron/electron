// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/window_list.h"

#include <algorithm>

#include "base/logging.h"
#include "browser/native_window.h"
#include "browser/window_list_observer.h"

namespace atom {

// static
base::LazyInstance<ObserverList<WindowListObserver>>::Leaky
    WindowList::observers_ = LAZY_INSTANCE_INITIALIZER;

// static
WindowList* WindowList::instance_ = NULL;

// static
WindowList* WindowList::GetInstance() {
  if (!instance_)
    instance_ = new WindowList;
  return instance_;
}

// static
void WindowList::AddWindow(NativeWindow* window) {
  DCHECK(window);
  // Push |window| on the appropriate list instance.
  WindowVector& windows = GetInstance()->windows_;
  windows.push_back(window);

  FOR_EACH_OBSERVER(WindowListObserver, observers_.Get(),
                    OnWindowAdded(window));
}

// static
void WindowList::RemoveWindow(NativeWindow* window) {
  WindowVector& windows = GetInstance()->windows_;
  windows.erase(std::remove(windows.begin(), windows.end(), window),
                windows.end());

  FOR_EACH_OBSERVER(WindowListObserver, observers_.Get(),
                    OnWindowRemoved(window));

  if (windows.size() == 0)
    FOR_EACH_OBSERVER(WindowListObserver, observers_.Get(),
                      OnWindowAllClosed());
}

// static
void WindowList::WindowCloseCancelled(NativeWindow* window) {
  FOR_EACH_OBSERVER(WindowListObserver, observers_.Get(),
                    OnWindowCloseCancelled(window));
}

// static
void WindowList::AddObserver(WindowListObserver* observer) {
  observers_.Get().AddObserver(observer);
}

// static
void WindowList::RemoveObserver(WindowListObserver* observer) {
  observers_.Get().RemoveObserver(observer);
}

// static
void WindowList::CloseAllWindows() {
  WindowVector windows = GetInstance()->windows_;
  for (size_t i = 0; i < windows.size(); ++i)
    windows[i]->Close();
}

WindowList::WindowList() {
}

WindowList::~WindowList() {
}

}  // namespace atom
