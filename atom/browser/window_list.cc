// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/window_list.h"

#include <algorithm>

#include "atom/browser/native_window.h"
#include "atom/browser/window_list_observer.h"
#include "base/logging.h"

namespace atom {

// static
base::LazyInstance<base::ObserverList<WindowListObserver>>::Leaky
    WindowList::observers_ = LAZY_INSTANCE_INITIALIZER;

// static
WindowList* WindowList::instance_ = nullptr;

// static
WindowList* WindowList::GetInstance() {
  if (!instance_)
    instance_ = new WindowList;
  return instance_;
}

// static
WindowList::WindowVector WindowList::GetWindows() {
  return GetInstance()->windows_;
}

// static
bool WindowList::IsEmpty() {
  return GetInstance()->windows_.empty();
}

// static
void WindowList::AddWindow(NativeWindow* window) {
  DCHECK(window);
  // Push |window| on the appropriate list instance.
  WindowVector& windows = GetInstance()->windows_;
  windows.push_back(window);

  for (WindowListObserver& observer : observers_.Get())
    observer.OnWindowAdded(window);
}

// static
void WindowList::RemoveWindow(NativeWindow* window) {
  WindowVector& windows = GetInstance()->windows_;
  windows.erase(std::remove(windows.begin(), windows.end(), window),
                windows.end());

  for (WindowListObserver& observer : observers_.Get())
    observer.OnWindowRemoved(window);

  if (windows.empty()) {
    for (WindowListObserver& observer : observers_.Get())
      observer.OnWindowAllClosed();
  }
}

// static
void WindowList::WindowCloseCancelled(NativeWindow* window) {
  for (WindowListObserver& observer : observers_.Get())
    observer.OnWindowCloseCancelled(window);
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
#if defined(OS_MACOSX)
  std::reverse(windows.begin(), windows.end());
#endif
  for (auto* const& window : windows)
    if (!window->IsClosed())
      window->Close();
}

// static
void WindowList::DestroyAllWindows() {
  WindowVector windows = GetInstance()->windows_;
  for (auto* const& window : windows)
    window->CloseImmediately();  // e.g. Destroy()
}

WindowList::WindowList() {}

WindowList::~WindowList() {}

}  // namespace atom
