// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/window_list.h"

#include <algorithm>

#include "base/logging.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list_observer.h"

namespace {
template <typename T>
std::vector<base::WeakPtr<T>> ConvertToWeakPtrVector(std::vector<T*> raw_ptrs) {
  std::vector<base::WeakPtr<T>> converted_to_weak;
  for (auto* raw_ptr : raw_ptrs) {
    base::WeakPtr<T> weak = raw_ptr->GetWeakPtr();
    converted_to_weak.push_back(weak);
  }
  return converted_to_weak;
}
}  // namespace

namespace electron {

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
  for (auto* const& window : windows) {
    if (window && !window->IsClosed())
      window->Close();
  }
}

// static
void WindowList::DestroyAllWindows() {
  WindowVector windows = GetInstance()->windows_;
  std::vector<base::WeakPtr<NativeWindow>> weak_windows =
      ConvertToWeakPtrVector(windows);

  for (const auto& window : weak_windows) {
    if (window)
      window->CloseImmediately();
  }
}

WindowList::WindowList() {}

WindowList::~WindowList() {}

}  // namespace electron
